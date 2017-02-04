<?php
#
# Copyright (c) 2016 Jeffrey M. Squyres.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

require_once "github-webhooks-common.inc";

##############################################################################

#
# Main processor for this github webhook: check email address of
# authors and commiters
#

function email_checker($commits_url, $json, $config, $opts, $commits)
{
    $happy = true;
    if (isset($config["ci-link-url"]) &&
        $config["ci-link-url"] != "") {
        $target_url = $config["ci-link-url"];
    }
    $debug_message = "checking URL: $commits_url\n\n";
    $final_message = "";

    $bad_addrs = $config["bad"];

    $i = 0;
    foreach ($commits as $key => $value) {
        if (!isset($value->{"sha"}) ||
            !isset($value->{"commit"}->{"author"}) ||
            !isset($value->{"commit"}->{"committer"}) ||
            !isset($value->{"commit"}->{"message"})) {
            my_die("Somehow commit infomation is missing from Github's response...");
        }

        $sha = $value->{"sha"};
        $author = $value->{"commit"}->{"author"}->{"email"};
        $committer = $value->{"commit"}->{"committer"}->{"email"};
        $repo = $json->{"repository"}->{"full_name"};
        $status_url = $config["api_url_base"] . "/repos/$repo/statuses/$sha";
        $debug_message .= "examining commit index $i / sha $sha:\nstatus url: $status_url\n";

        $status = array(
            "context" => $config["ci-context"]
        );

        # Look for a bozo commit email address
        $bozo = 0;
        $bozo_addr = "";
        foreach (array("author", "committer") as $id) {
            foreach ($bad_addrs as $ba_value) {
                if (!$bozo) {
                    $bozo = preg_match("$ba_value", ${$id});
                    $bozo_addr = ${$id};
                }
            }
        }
        if (!$bozo) {
            $status["state"]       = "success";
            $status["description"] = "This commit has good email addresses. Yay!";
            $debug_message .= "$author / $committer -- looks good!\n";
        } else {
            $status["state"]       = "failure";
            $status["description"] = "Bozo $id email address: $bozo_addr.";
            if (isset($target_url)) {
                $status["target_url"]  = $target_url;
            }
            $debug_message .= $status["description"] . "\n";

            $happy = false;
        }

        # If this is the last commit in the array (and there's more than
        # one commit in the array), override its state and description to
        # represent the entire PR (because Github shows the status of the
        # last commit as the status of the overall PR).
        if ($i == count($commits) - 1 && $i > 0) {
            if ($happy) {
                $status["state"]       = "success";
                $status["description"] = "All commits have good email addresses. Yay!";
            } else {
                $status["state"]       = "failure";
                $status["description"] = "Some commits have bozo email addresses.";
                if (isset($target_url)) {
                    $status["target_url"]  = $target_url;
                }
            }
            $final_message = $status["description"];
        }
        # Send the results back to Github for this specific commit
        $ch = open_curl($status_url, $config);
        curl_setopt($ch, CURLOPT_POST, 1);
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($status));
        $output = curl_exec($ch);
        curl_close($ch);

        $debug_message .= "Beginning of curl output: " . substr($output, 0, 256) . "\n";

        ++$i;
    }

    debug($config, "$debug_message\n");
    printf("$final_message\n");

    # Happiness!  Exit.
    exit(0);
}

##############################################################################
# Main

webhook_main("commit-email-checker-config.inc", "email_checker");
