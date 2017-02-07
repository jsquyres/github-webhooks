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

function signed_off_by_checker($commit, $config, &$msg_out)
{
    if (preg_match("/Signed-off-by: /", $commit->{"commit"}->{"message"})) {
        return true;
    } else {
        $msg_out = $config["gwc one bad msg"];
        return false;
    }
}

##############################################################################
# Main

$config["gwc user config file"] = "signed-off-by-checker-config.inc";
$config["gwc check all function"] = "gwc_check_all_commits";
$config["gwc check one function"] = "signed_off_by_checker";

$config["gwc one good msg"] = "Commit is signed off.  Yay!";
$config["gwc all good msg"] = "All commits signed off.  Yay!";
$config["gwc one bad msg"] = "Commit not signed off";
$config["gwc some bad msg"] = "Some commits not signed off";
$config["gwc all bad msg"] = "No commits signed off";

gwc_webhook_main($config);
