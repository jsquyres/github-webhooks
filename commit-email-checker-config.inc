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
    $config["ci-context"] = "Commit email checker";

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
    #$config["github"]["jsquyres/github-webhooks"] = 1;
    $config["github"]["jsquyres/*"] = 1;

    # GitHub API URL.  Can be configured for internal / GitHub
    # Enterprise instances.
    #
    # GitHub.com: https://api.github.com
    # GHE: http[s]://YOUR_GHE_SERVER/api/v3
    #
    $config["api_url_base"] = "https://api.github.com";

    #####################################################################
    # Config specific to the bozo email checker
    #####################################################################

    # You can define any of the "bad", "good", and/or "hooks" values,
    # or leave them undefined (although you probably want to define at
    # least one of them!).

    # Array of regular expressions used to check author/committer
    # email addresses.  These are Perl-style regular expressions
    # (i.e., they are passed to the PHP function preg_match()).  If
    # any of these match, fail the test.
    $config["bad"] = array(
        "/^root@/",
        "/localhost/",
        "/localdomain/");

    # Array of regular expressions used to check author/committer
    # email addresses.  These are Perl-style regular expressions
    # (i.e., they are passed to the PHP function preg_match()).  If
    # any of these DO NOT match, fail the test (i.e., all of them must
    # match to pass the test).
    $config["good"] = array(
        '/^(\w+)\@mydomain\.com$/i');

    # Array function names to be called for each commit (i.e., any
    # custom code you want to check).  Note that these hooks are only
    # called if the above good/bad checks pass.
    $config["hooks"] = array(
        "my_email_check_function");

    # Return the $config variable
    return $config;
}

# Called from $config["hooks"].  $id will be either "author" or
# "committer", and $email will be the email address.
#
# If you return true, the test passed (yay!).  If you return false,
# the test failed (boo!).
function my_email_check_function($id, $email)
{
    # ...do some meaningful check here...
    return true;
}
