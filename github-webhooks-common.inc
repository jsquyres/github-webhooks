<?php
#
# Copyright (c) 2016-2017 Jeffrey M. Squyres.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

#
# Common helpers for all the github webhooks
#

# For 4.3.0 <= PHP <= 5.4.0
if (!function_exists('http_response_code')) {
    function http_response_code($newcode = NULL) {
        static $code = 200;
        if ($newcode !== NULL) {
            header("X-PHP-Response-Code: $newcode", true, $newcode);
            if (!headers_sent()) {
                $code = $newcode;
            }
        }
        return $code;
    }
}

function my_die($msg, $code = 400)
{
    # Die with a non-200 error code
    http_response_code($code);

    die($msg);
}

function debug($config, $str)
{
    if (isset($config["debug"]) && $config["debug"]) {
        print($str);
    }
}

##############################################################################

function check_for_allowed_sources($config)
{
    global $config;

    if (isset($config["allowed_sources"]) &&
        count($config["allowed_sources"] > 0)) {
        if (isset($_SERVER["HTTP_X_REAL_IP"])) {
            $source_str = $_SERVER["HTTP_X_REAL_IP"];
            $source = ip2long($_SERVER["HTTP_X_REAL_IP"]);
        } else if (isset($_SERVER["REMOTE_ADDR"])) {
            $source_str = $_SERVER["REMOTE_ADDR"];
            $source = ip2long($_SERVER["REMOTE_ADDR"]);
        } else {
            # This will not match anything
            $source = 0;
        }

        $happy = 0;
        foreach ($config["allowed_sources"] as $cidr) {
            $parts = explode('/', $cidr);
            $value = ip2long($parts[0]);
            $mask = (pow(2, 33) - 1) - (pow(2, $parts[1] + 1) - 1);

            if (($value & $mask) == ($source & $mask)) {
                $happy = 1;
            }
        }
        if (!$happy) {
            my_die("Discarding request from disallowed IP address ($source_str)\n");
        }
    }
}

function get_payload()
{
    # Ensure we got a non-empty payload
    if (isset($_POST["payload"])) {
        return $_POST["payload"];
    } else {
        $payload = file_get_contents("php://input");
        if ($payload != "") {
            return $payload;
        }
    }

    my_die("Received POST request with empty payload\n");
}

##############################################################################

function parse_json($json_string)
{
    # Parse the JSON
    $json = json_decode($json_string);
    if (json_last_error() != JSON_ERROR_NONE) {
        my_die("Got invalid JSON\n");
    }

    return $json;
}

function fill_opts_from_json($json)
{
    # If this is just a github ping, we can ignore it
    if (!isset($json->{"action"}) ||
        ($json->{"action"} != "synchronize" &&
         $json->{"action"} != "opened")) {
       print "Hello, Github ping!  I'm here!\n";
       exit(0);
    }

    $opts["repo"] = $json->{"repository"}->{"full_name"};

    return $opts;
}

function fill_opts_from_keys($config, $opts, $arr)
{
    # Deep copy the keys/values into the already-existing $opts
    # array
    if (is_array($arr)) {
        foreach ($arr as $k => $v) {
            $opts[$k] = $v;
        }
    }

    # Was the URL set?
    if (!isset($opts["uri"]) && isset($config["url"])) {
        $opts["uri"] = $config["url"];
    }

    return $opts;
}

##############################################################################

function open_curl($url, $config)
{
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_USERAGENT, $config["ci-context"]);

    # Setting CURLOPT_RETURNTRANSFER variable to 1 will force curl not to
    # print out the results of its query.  Instead, it will return the
    # results as a string return value from curl_exec() instead of the
    # usual true/false.
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

    # Set CURLOPT_FOLLOWOCATION so that if we get any redirects, curl
    # will follow them to get the content from the final location.
    curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);

    # Set CURLOPT_HTTPHEADER to include the authorization token, just
    # in case this is a private repo.
    $headers = array(
        "Content-type: application/json",
        "Authorization: token " . $config["auth_token"]
    );
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);

    return $ch;
}

function get_commits($commits_url, $config)
{
    # This webhook will have only delivered the *new* commits on this
    # PR.  We need to examine *all* the commits on this PR -- so we
    # can discard the commits that were delivered in this webhook
    # payload.  Instead, do a fetch to get all the commits on this PR
    # (note: putting the Authorization header in this request just in
    # case this is a private repo).

    $ch = open_curl($commits_url, $config);
    $output = curl_exec($ch);

    # Check to see if we got success
    if (curl_errno($ch)) {
        curl_close($ch);
        my_die("Sorry, something went wrong while trying to obtain the URL \"$commits_url\".");
    }
    curl_close($ch);

    $commits = parse_json($output);

    # Sanity check
    if (count($commits) == 0) {
        my_die("Somehow there are no commits on this PR... weird...");
    }

    return $commits;
}

function repo_matches($full_name, $key)
{
    if ($full_name == $key) {
        return 1;
    }

    // Full wildcard
    else if ($key == "*" ||
             $key == "*/*") {
        return 1;
    }

    // Partial wildcards
    preg_match("/^(.+?)\/(.+)$/", $full_name, $name_matches);
    preg_match("/^(.+?)\/(.+)$/", $key, $key_matches);
    if ($key_matches[1] == "*" && $key_matches[2] == $name_matches[2]) {
        return 1;
    }
    else if ($key_matches[2] == "*" && $key_matches[1] == $name_matches[1]) {
        return 1;
    }

    return 0;
}

function process($json, $config, $opts, $value, $process_fn)
{
    $opts = fill_opts_from_keys($config, $opts, $value);

    $commits_url = $json->{"pull_request"}->{"commits_url"};
    $commits = get_commits($commits_url, $config);
    $process_fn($commits_url, $json, $config, $opts, $commits);
}

##############################################################################

# Main (called by each of the individual webhook .php files)

function webhook_main($config_filename, $process_fn)
{
    # Verify that this is a POST
    if (!isset($_SERVER["REQUEST_METHOD"]) ||
    $_SERVER["REQUEST_METHOD"] != "POST") {
        print("Use " . $_SERVER["SERVER_NAME"] .
            $_SERVER["REQUEST_URI"] .
            ":" . $_SERVER["SERVER_PORT"] .
            " as a WebHook URL in your Github repository settings.\n");
        exit(1);
    }

    # Read the config
    $config = fill_config($config_filename);

    # Sanity checks
    check_for_allowed_sources($config);
    $payload = get_payload();

    $json = parse_json($payload);
    $opts = fill_opts_from_json($json);

    # Loop over all the repos in the config; see if this incoming
    # request is from one we recognize.  The keys of $config["github"]
    # are repo names (e.g., "jsquyres/github-webhooks").
    $repo = $json->{"repository"}->{"full_name"};
    foreach ($config["github"] as $key => $value) {
        if (repo_matches($repo, $key)) {
            process($json, $config, $opts, $value);

            # process() will not return, but be paranoid anyway
            exit(0);
        }
    }

    # If we get here, it means we didn't find a repo match
    my_die("Sorry; $repo is not a Github repo for which I provide service");
}
