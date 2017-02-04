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

function email_checker($commit, $config, &$msg_out)
{
    $author = $commit->{"commit"}->{"author"}->{"email"};
    $committer = $commit->{"commit"}->{"committer"}->{"email"};

    foreach (array("author", "committer") as $id) {
        foreach ($config["bad"] as $ba_value) {
            if (preg_match("$ba_value", ${$id})) {
                $msg_out = "Bad $id email: ${$id}";
                return false;
            }
        }
    }

    return true;
}

##############################################################################
# Main

$config["gwc user config file"] = "commit-email-checker-config.inc";
$config["gwc check all function"] = "gwc_check_all_commits";
$config["gwc check one function"] = "email_checker";

$config["gwc one good msg"] = "Good email address.  Yay!";
$config["gwc all good msg"] = "All commits have good email addresses.  Yay!";
$config["gwc one bad msg"] = "Bad email address";
$config["gwc some bad msg"] = "Some commits have bad email addresses";
$config["gwc all bad msg"] = "No commits have sane email addresses";

gwc_webhook_main($config);
