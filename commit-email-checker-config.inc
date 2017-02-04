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
# This file must contain the function fill_config().
##############################################################################
##############################################################################

function fill_config()
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
    $config["ci-context"] = "Bozo email checker";

    # Optional: if a commit contains a boneheaded email address, link
    # to this URL
    #$config["ci-link-url"] = "http://example.com";

    # You almost certainly want "debug" set to 0 (or unset)
    $config["debug"] = 0;

    # These are the repos for which we are allowed to reply.
    # Valid values are:
    # - a full repo name, such as: jsquyres/email-checker-github-webhook
    # - full wildcard: *
    # - wildcard the username/org name: */email-checker-github-webhook
    # - wildcard the repo name: jsquyres/*
    # - wildcard both names: */*
    #$config["github"]["jsquyres/email-checker-github-webhook"] = 1;
    $config["github"]["jsquyres/*"] = 1;

    # GitHub API URL.  Can be configured for internal / GitHub
    # Enterprise instances.
    #
    # GitHub.com: https://api.github.com
    # GHE: http[s]://YOUR_GHE_SERVER/api/v3
    #
    $config["api_url_base"] = "https://api.github.com";

    # Array of regular expressions used to check author/committer
    # email addresses.  These are Perl-style regular expressions
    # (i.e., they are passed to the PHP function preg_match()).
    $config["bad"] = array(
        "/^root@/",
        "/localhost/",
        "/localdomain/");

    # Return the $config variable
    return $config;
}
