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
# Main processor for this github webhook: check that each commit has a
# "Signed-off-by" line.
#

function signed_off_by_checker($commits_url, $json, $config, $opts, $commits)
{
    $happy = true;
    if (isset($config["ci-link-url"]) &&
        $config["ci-link-url"] != "") {
        $target_url = $config["ci-link-url"];
    }
    $debug_message = "checking URL: $commits_url\n\n";
    $final_message = "";

    $i = 0;
    foreach ($commits as $key => $value) {
        if (!isset($value->{"sha"}) ||
            !isset($value->{"commit"}) ||
            !isset($value->{"commit"}->{"message"})) {
            my_die("Somehow commit infomation is missing from Github's response...");
        }

        $sha = $value->{"sha"};
        $message = $value->{"commit"}->{"message"};
        $repo = $json->{"repository"}->{"full_name"};
        $status_url = $config["api_url_base"] . "/repos/$repo/statuses/$sha";
        $debug_message .= "examining commit index $i / sha $sha:\nstatus url: $status_url\n";

        $status = array(
            "context" => $config["ci-context"]
        );

        # Look for a Signed-off-by string in this commit
        if (preg_match("/Signed-off-by/", $message)) {
            $status["state"]       = "success";
            $status["description"] = "This commit is signed off. Yay!";
            $debug_message .= "This commit is signed off\n\n";
        } else {
            $status["state"]       = "failure";
            $status["description"] = "This commit is not signed off.";
            if (isset($target_url)) {
                $status["target_url"]  = $target_url;
            }
            $debug_message .= "This commit is NOT signed off\n\n";

            $happy = false;
        }
        $final_message = $status["description"];

        # If this is the last commit in the array (and there's more than
        # one commit in the array), override its state and description to
        # represent the entire PR (because Github shows the status of the
        # last commit as the status of the overall PR).
        if ($i == count($commits) - 1 && $i > 0) {
            if ($happy) {
                $status["state"]       = "success";
                $status["description"] = "All commits signed off. Yay!";
            } else {
                $status["state"]       = "failure";
                $status["description"] = "Some commits not signed off.";
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

        ++$i;
    }

    debug($config, "$debug_message\n");
    printf("$final_message\n");

    # Happiness!  Exit.
    exit(0);
}


##############################################################################
# Main

webhook_main("signed-off-by-checker-config.inc", "signed_off_by_checker");
