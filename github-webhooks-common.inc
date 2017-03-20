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

##############################################################################

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

##############################################################################

function gwc_die($msg, $code = 400)
{
    # Die with a non-200 error code
    http_response_code($code);

    die($msg);
}

function gwc_debug($config, $str)
{
    if (isset($config["debug"]) && $config["debug"]) {
        print date('Y-m-d H:i:s: ');
        print($str);
    }
}

##############################################################################

function gwc_check_for_allowed_sources($config)
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
            gwc_die("Discarding request from disallowed IP address ($source_str)\n");
        }
    }
}

function gwc_get_payload()
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

    gwc_die("Received POST request with empty payload\n");
}

##############################################################################

function gwc_parse_json($json_string)
{
    # Parse the JSON
    $json = json_decode($json_string);
    if (json_last_error() != JSON_ERROR_NONE) {
        gwc_die("Got invalid JSON\n");
    }

    return $json;
}

function gwc_fill_opts_from_json($json)
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

function gwc_fill_opts_from_keys($config, $opts, $arr)
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

function gwc_open_curl($url, $config)
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

function gwc_get_commits($commits_url, $num_commits, $config)
{
    # This webhook will have only delivered the *new* commits on this
    # PR.  We need to examine *all* the commits on this PR -- so we
    # can discard the commits that were delivered in this webhook
    # payload.  Instead, do a fetch to get all the commits on this PR
    # (note: putting the Authorization header in this request just in
    # case this is a private repo).

    # Note that Github will only deliver a "page" full of commits at a
    # time (as of Mar 2017, a "page" is 30 commits).  If the PR
    # contains more than 30 commits, we have to make multiple calls to
    # get them all.

    $got = 0;
    # Github pagination starts with page 1
    $page = 1;
    gwc_debug($config, "Looking for $num_commits commits...\n");
    while ($got < $num_commits) {
        gwc_debug($config, "Getting commits: $commits_url?page=$page\n");
        $ch = gwc_open_curl("$commits_url?page=$page", $config);
        $output = curl_exec($ch);

        # Check to see if we got success
        if (curl_errno($ch)) {
            curl_close($ch);
            gwc_die("Sorry, something went wrong while trying to obtain the URL \"$commits_url\".");
        }
        curl_close($ch);

        $commits_tmp = gwc_parse_json($output);

        # Sanity check
        if (count($commits_tmp) == 0) {
            gwc_die("Somehow there are no commits on this PR... weird...");
        }

        gwc_debug($config, "Page $page had " . count($commits_tmp) . " commits\n");

        foreach ($commits_tmp as $key => $value) {
            $commits[$got] = $value;
            ++$got;
        }
        gwc_debug($config, "Total of " . count($commits) . " so far\n");

        ++$page;
    }

    gwc_debug($config, "Found a total of " . count($commits) . " commits\n");
    return $commits;
}

##############################################################################

function gwc_repo_matches($full_name, $key)
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

##############################################################################

function gwc_set_github_commit_status($config, $status_url, $status)
{
    $ch = gwc_open_curl($status_url, $config);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($status));
    $output = curl_exec($ch);
    curl_close($ch);
}

##############################################################################

function gwc_check_all_commits($commits_url, $json, $config, $opts, $commits)
{
    if (isset($config["ci-link-url"]) &&
        $config["ci-link-url"] != "") {
        $target_url = $config["ci-link-url"];
    }
    $debug_message = "checking URL: $commits_url\n\n";
    $final_message = "";

    if (isset($config["max num statuses to set"])) {
        $max_status_set_count = $config["max num statuses to set"];
    } else {
        $max_status_set_count = 30;
    }
    if (isset($config["set only bad+final"])) {
        $set_only_bad_and_final = $config["set only bad+final"];
    } else {
        $set_only_bad_and_final = true;
    }

    $i = 0;
    $good_count = 0;
    $bad_count = 0;
    $status_set_count = 0;
    foreach ($commits as $key => $value) {
        if (!isset($value->{"sha"}) ||
            !isset($value->{"commit"}) ||
            !isset($value->{"commit"}->{"message"})) {
            gwc_die("Somehow commit information is missing from Github's response...");
        }

        $message = $value->{"commit"}->{"message"};

        $sha = $value->{"sha"};
        $repo = $json->{"repository"}->{"full_name"};
        $status_url = $config["api_url_base"] . "/repos/$repo/statuses/$sha";
        $debug_message .= "examining commit index $i / sha $sha:\nstatus url: $status_url\n";

        $status = array(
            "context" => $config["ci-context"]
        );

        # Call the "check one commit" function to see if this commit
        # is good or bad.
        $msg = "";
        $good = $config["gwc check one function"]($value, $config, $msg);

        # Process the result of that message
        if ($good) {
            ++$good_count;
            $status["state"]       = "success";
            if ($msg != "") {
                $status["description"] = $msg;
            } else {
                $status["description"] = $config["gwc one good msg"];
            }
        } else {
            ++$bad_count;
            $status["state"]       = "failure";
            if ($msg != "") {
                $status["description"] = $msg;
            } else {
                $status["description"] = $config["gwc one bad msg"];
            }
            if (isset($target_url)) {
                $status["target_url"]  = $target_url;
            }

            $happy = false;
        }
        $debug_message .= "This commit is: ". $status["description"] . "\n";;
        $final_message = $status["description"];

        # If this is the last commit in the array (and there's more than
        # one commit in the array), override its state and description to
        # represent the entire PR (because Github shows the status of the
        # last commit as the status of the overall PR).
        if ($i == count($commits) - 1 && $i > 0) {
            if ($good_count == count($commits)) {
                $status["state"]       = "success";
                $status["description"] = $config["gwc all good msg"];
            } else {
                $status["state"]       = "failure";
                if ($bad_count == count($commits)) {
                    $status["description"] = $config["gwc all bad msg"];
                } else {
                    $status["description"] = $config["gwc some bad msg"];
                }
                if (isset($target_url)) {
                    $status["target_url"]  = $target_url;
                }
            }
            $final_message = $status["description"];
        }

        # Send the results back to Github for this specific commit
        # (...or not)
        $debug_message .= "Setting status on this commit: ";
        if ($i == count($commits) - 1) {
            $debug_message .= "YES\nReason: it is the last commit\n";
            gwc_set_github_commit_status($config, $status_url, $status);
        } else if ($status_set_count > $max_status_set_count) {
            $debug_message .= "NO\nReason: we've set too many already\n";
        } else if (!$good) {
            $debug_message .= "YES\nReason: it is BAD!\n";
            gwc_set_github_commit_status($config, $status_url, $status);
        } else if (!$set_only_bad_and_final) {
            $debug_message .= "YES\nReason: we're setting on *all* commits\n";
            gwc_set_github_commit_status($config, $status_url, $status);
        } else {
            $debug_message .= "NO\nReason: we're only setting on the bad+last commits\n";
        }
        $debug_message .= "\n";

        ++$i;
    }

    gwc_debug($config, "$debug_message");
    printf("Final evaluation: $final_message\n");

    # Happiness!  Exit.
    exit(0);
}

##############################################################################

# Sanity check

function gwc_verify_post()
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
}

function gwc_verify_config($config)
{
    $a = array("gwc user config file",
               "gwc check all function",
               "gwc check one function");
    foreach ($a as $key => $value) {
        if (!isset($config[$value])) {
            gwc_die("Something weird wrong: missing '$value' in internal configuration");
        }
    }

    if (!isset($config["gwc one good msg"])) {
        $config["gwc one good msg"] = "Good commit. Yay!";
    }
    if (!isset($config["gwc all good msg"])) {
        $config["gwc all good msg"] = "All commits good. Yay!";
    }
    if (!isset($config["gwc one bad msg"])) {
        $config["gwc one bad msg"] = "Bad commit";
    }
    if (!isset($config["gwc some bad msg"])) {
        $config["gwc some bad msg"] = "Some commits bad";
    }
    if (!isset($config["gwc all bad msg"])) {
        $config["gwc all bad msg"] = "All commits bad";
    }

    return $config;
}

function gwc_verify_function($name, $msg = "")
{
    if (!function_exists($name)) {
        if ($msg != "") {
            gwc_die($msg);
        } else {
            gwc_die("Cannot find the $name() function.  I'm just going to lie dwne here, quietly, in the snow for a while...");
        }
    }
}

function gwc_load_user_config($config)
{
    # Sanity check
    $config_filename = $config["gwc user config file"];
    if (!is_file($config_filename)) {
        gwc_die("Cannot find $config_filename config file.");
    }

    # Include the user config file
    require_once $config_filename;

    # Make sure that user config file defined the
    # "fill_webhook_config" function
    gwc_verify_function("fill_webhook_config",
                        "Cannot find the fill_webhook_config() function in the $config_filename file.");

    # Run the user config function to get their data
    $user_config = fill_webhook_config();

    # Merge the script-provided config and user-provided config together
    foreach ($user_config as $key => $value) {
        $config[$key] = $value;
    }

    return $config;
}

# Main (called by each of the individual webhook .php files)

function gwc_webhook_main($config)
{
    # Sanity checks
    gwc_verify_post();
    $config = gwc_verify_config($config);
    gwc_verify_function($config["gwc check all function"]);
    gwc_verify_function($config["gwc check one function"]);

    # Load the config file
    $config = gwc_load_user_config($config);

    # Ensure that we abide by allowed sources
    gwc_check_for_allowed_sources($config);

    $payload = gwc_get_payload();
    $json = gwc_parse_json($payload);
    $opts = gwc_fill_opts_from_json($json);

    # Loop over all the repos in the config; see if this incoming
    # request is from one we recognize.  The keys of $config["github"]
    # are repo names (e.g., "jsquyres/github-webhooks").
    $repo = $json->{"repository"}->{"full_name"};
    foreach ($config["github"] as $key => $value) {
        if (gwc_repo_matches($repo, $key)) {
            gwc_debug($config, "repo matches: $repo / $key\n");

            # Get the JSON of the commits on this PR
            $commits_url = $json->{"pull_request"}->{"commits_url"};
            $num_commits = $json->{"pull_request"}->{"commits"};
            $commits = gwc_get_commits($commits_url, $num_commits, $config);

            # Process the commits
            $opts = gwc_fill_opts_from_keys($config, $opts, $value);
            $config["gwc check all function"]($commits_url, $json, $config,
                              $opts, $commits);

            # the process function should hopefully not return, but be
            # paranoid anyway
            exit(0);
        }
    }

    # If we get here, it means we didn't find a repo match
    gwc_die("Sorry; $repo is not a Github repo for which I provide service.");
}
