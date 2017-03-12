# Some GitHub webhooks

These are simple, pure-PHP Github webhooks.

# Why does this project exist?

This project exists as simple, standalone PHP scripts so that you can
run them in web hosting environments where you are unable to run
additional daemons.  For example, many simple web hosting packages
allow arbitrary PHP web pages, but do not allow running standalone
processes (such as a Ruby / Sinatra process) for more than a short
period of time.

# Installation

Installation is comprised of two parts:

1. Installation on the web server
1. Installation at github.com or GitHub Enterprise

## Installation on the web server

1. Copy the source files to a folder in a docroot somewhere.
   * Edit the `*-config.inc` files to reflect the configuration that
     you want.  There are comments in these files explaining each of
     the options.
   * You will need to set the name(s) of the GitHub repos for which
     you want the script to respond.
   * You will also need to set the `auth_token` value to be the
     Personal Access Token (PAT) of a user who has commit access to each of
     these repos.
     * You can generate a Github PAT by visiting
       https://github.com/settings/tokens and clicking the "Generate
       new tokens" button in the top right (or
       https://YOUR_GHE_SERVER/settings/tokens).
     * For public repositories, the only permission that this PAT
       needs is `repo:status`.
     * For private repositories, it is important to select the outer
       `repo` permission ("full control of private repositories").
       This will automatically check all the sub-repo permissions as
       well.  It is *not* sufficient to simply check all the sub-repo
       permissions and leave the outer `repo` status unchecked.
1. Configure your web server to deny client access to the
   `*.inc` files.
   * ***THIS IS NOT OPTIONAL***
   * Failure to do so will expose your PAT!
   * For example, you can create a `.htaccess` file in the same
     directory to restrict access to your `*.inc` files containing:
```xml
<Files "*\.inc">
    Order allow,deny
    Deny from all
    Satisfy all
</Files>
```

## Installation at github.com / GitHub Enterprise

1. On Github.com, create a custom webhook for your Git repo:
   * The URL should be the URL of either of your newly-installed `.php` files (e.g., `https://SERVER_NAME/PATH/signed-off-by-checker.php`).
   * The content type should be `application/json`.
   * Select "Let me select individual events."
     * Check the "Pull request" event.
     * You can choose to leave the "Push" event checked or not.
   * Make the webhook active.
1. When you create a webhook at github.com / your GHE server, it should send a "ping" request to the webhook to make sure it is correct.
   * On your git repository's webhooks page, click on your webhook.
   * On the resulting page, scroll down to the "Recent Deliveries" section.  The very first delivery will be the "ping" event.
   * Click on the first delivery and check that the response code is 200.
   * If everything is working properly, the output body should say
     `Hello, Github ping!  I'm here!`.
   * If you do not see the ping results, see below.
1. Create a pull request to your Git repository.
    * If all goes well, if your commits meet the criteria of the
      webhook (e.g., if they all contain `Signed-off-by` tokens), you
      should get a green check for each commit in the PR.
   * If any commit(s) do *not* meet the webhook criteria, then that
     that(those) commit(s) should get a red X, and the *last* commit
     should also get a red X.
   * If you don't see the CI results on the pull request, see below.

## Troubleshooting

Common problems include:

* Not seeing the Github ping.
  * Check that the URL in your Github webhook is correct.
  * If using an `https` URL, check that the certificate is
    authenticate-able by Github (i.e., it's signed by a recognizable
    CA, etc.)
  * Try test surfing to the URL of your Github webhook yourself and
    make sure it is working properly.  You should see:
```
Use /PATH/*.php as a WebHook URL in your Github repository settings.
````

* Not seeing the checker's results in the CI block on the PR
  * Ensure that the PAT used is from a user that has commit access to
    the repository.  If the PHP script is running properly and the
    results don't appear in the PR CI section, *this is almost always
    the problem*.
  * Check the "Recent deliveries" section of your webhook's
    configuration page and check the response body output from your
    delivery.
  * Click on a delivery hash to expand it; click the Response tab to
    see the output from the PHP script.
  * If all goes well, the last line of output should show a message
    indicating that it has seen all the commits in the PR and all of
    them passed.

# The Webhooks

There are currently two webhooks available:

1. "Signed-off-by" checker
2. Committer and author email checker

## Signed-off-by checker

This webhook does a simple check: it makes sure that each commit in
the PR contains the `Signed-off-by` token.  If a commit does not
contain this token, it fails the check.

### Expanding the signed-off-by checker

You can expand this webhook easily.

For example, it may be desirable to look for specific signoffs.  E.g.,
you might want to look for signoffs from specific team leads when
junior developers make PRs.

## Committer and author email checker

This webhook checks for common errors in commit email addresses, and
is easily expandable to include your own checks.

By default, this webhook checks that the commiter and author email addresses on each command do not contain:

1. `root@`
1. `localhost`
1. `localdomain`

These three checks tend to catch newbie Git users who have forgotten
to set their username and email address in their Git configuration.

### Expanding the email checker

It may also be desirable to ensure other qualities about the committer
and/or author emails.  For example:

1. Ensure that the committer is a current employee in your
   organization (we do this in my group at work).
1. Ensure that both the author and committer use correct email address
   (e.g., `username@mycompany.com` instead of
   `username@some.internal.machine.name.mycompany.com`).

And so on.

# History

These two GitHub webhooks started their lives in separate GitHub
repositories:

* https://github.com/jsquyres/signed-off-by-github-webhook
* https://github.com/jsquyres/email-checker-github-webhook

However, a huge chunk of code was shared between the two of them, so
it made sense to unify the two into a single code base and a single
repo.
