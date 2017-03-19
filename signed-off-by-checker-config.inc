<?php
#
# Copyright (c) 2017 Jeffrey M. Squyres.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

##############################################################################
##############################################################################
# This file must contain the function fill_webhook_config().
##############################################################################
##############################################################################

function fill_webhook_config()
{
    # git-commit-email-checker.php will ignore any incoming POST that does
    # not come from these network blocks.
    #
    # For GitHub Enterprise, fill in the CIDR notation for your GitHub
    # server(s).
    #$config["allowed_sources"] = array("10.10.11.0/24", "10.10.22.0/24");
    #
    # For GitHub.com, as of 9 Aug 2016, according to
    # https://help.github.com/articles/what-ip-addresses-does-github-use-that-i-should-whitelist/:
    $config["allowed_sources"] = array("192.30.252.0/22");

    # Fill this in with a Github personal access token that is allowed
    # to set pull request statuses.  See the README.md for more detail.
    $config["auth_token"] = "Fill me in";

    # The name that will appear for this CI agent at GitHub on pull
    # requests.
    $config["ci-context"] = "Signed-off-by checker";

    # Optional: if a commit contains a commit without a
    # "Signed-off-by", link to this URL
    #$config["ci-link-url"] = "http://example.com";

    # You almost certainly want "debug" set to 0 (or unset)
    $config["debug"] = 0;

    # Do you want to set the status on *every* commit to be "good" or
    # bad?  Or do you want to only set the status on *bad* commits and
    # the *last* commit?  For large pull requests (e.g., over 60 or 80
    # commits), setting the status on *every* commit can be quite
    # slow, and github may timeout the webhook before it completes.
    $config["set only bad+final"] = true;

    # Failsafe: in *ALL* cases (regardless of the value of "set only
    # bad+final"), only set this number of statuses (plus the last
    # status).  This prevents Github from timing out this webhook if
    # someone creates a PR with a large number of bad commits (and
    # therefore this webhook doesn't get the chance to set the
    # final/PR-wide status).  It is risky to set this number much
    # higher than 30.
    $config["max num statuses to set"] = 30;

    # These are the repos for which we are allowed to reply.
    # Valid values are:
    # - a full repo name, such as: jsquyres/email-checker-github-webhook
    # - full wildcard: *
    # - wildcard the username/org name: */email-checker-github-webhook
    # - wildcard the repo name: jsquyres/*
    # - wildcard both names: */*
    #$config["github"]["jsquyres/github-webhooks"] = 1;
    $config["github"]["jsquyres/*"] = 1;

    # GitHub API URL.  Can be configured for internal / GitHub
    # Enterprise instances.
    #
    # GitHub.com: https://api.github.com
    # GHE: http[s]://YOUR_GHE_SERVER/api/v3
    #
    $config["api_url_base"] = "https://api.github.com";

    # Return the $config variable
    return $config;
}
