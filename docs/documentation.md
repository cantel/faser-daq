# Documentation
Here we go, time for meta-documentation.  If you have made it this far then bravo.  Pat
yourself on the back and crack a beer because you deserve it.  Good documentation provides
the knowhow to others to be able to work effectively, and you have arrived here because you
presumably want to improve it - GREAT! There are a couple ways to improve this documentation
depending on how much you want to sink into it.

## How is this built?
This documentation is build using [mkdocs](https://www.mkdocs.org/) within [GitLab Pipelines@CERN](https://cern.service-now.com/service-portal?id=kb_article&n=KB0003905) and then 
pushed to and housed on [EOS@CERN](https://gitlab.cern.ch/gitlabci-examples/deploy_eos).  

The documentation is housed at [https://gitlab.cern.ch/meehan/faser-docs-gitlab](https://gitlab.cern.ch/meehan/faser-docs-gitlab)
and the top level configuration file is that of [mkdocs.yml](https://gitlab.cern.ch/meehan/faser-docs-gitlab/-/blob/master/mkdocs.yml).
This more or less just organizes the utilities and the high level structure of the website
but the actual documentation is written in markdown and is contained within the `.md` files
contained within the `docs` directory and inserted into the navigation tree.

In addition to the basic `mkdocs` configuration, a number of plugins are used as can be seen
in the top level `mkdocs.yml`.  These plugins are installed upon each build in the CI from
the locations listed on the [requirements.txt](https://gitlab.cern.ch/meehan/faser-docs-gitlab/-/blob/master/requirements.txt) file.

## How can I improve it?
There are a few different ways to improve the documentation and we encourage everyone to do
so in the manner that suits them best.

### Submit a Merge Request
The recommended way for those that have a bit of time is to develop and submit a MR to
the [https://gitlab.cern.ch/meehan/faser-docs-gitlab](https://gitlab.cern.ch/meehan/faser-docs-gitlab)
repository.  This can be done, for example, by developing a branch and then submitting from
that branch, similar to that described for the `faser/daq` code base itself.

This has the added benefit that you can use the procedure outlined at the bottom of this page
to build the website locally on your machine to preview what your changes will look like.
This can be especially useful if the change is rather involved.

### Edit Directly in GitLab
If the change is minor, then it is always possible to edit the relevant file directly
in the GitLab web GUI.  

  - Navigate to the file you wish to modify.
  - Click on the `Edit` icon to open the GitLab GUI editor.
  - Make your changes.
  - Submit the changes with the `Open new Merge Request` box selected
  
This will submit a new MR that you will have to complete normally.

Likewise, if you need to modify multiple files, you can open the [`Web IDE`](https://docs.gitlab.com/ee/user/project/web_ide/) browser
and make all of your changes to be submitted in one MR.

### Submit an Issue
If you see something, but don't have the time to actually fix it and can't be bothered
to open a MR, then we encourage you to [open an issue](https://gitlab.cern.ch/meehan/faser-docs-gitlab/-/issues)
and point out the change that you want to make.  Someone will get around to doing it eventually
and this ensures that the modification/fix/improvement will not be lost.

## Building Locally
It is possible to build all of this locally as described on [the mkdocs documentation](https://www.mkdocs.org/#building-the-site).
In short, after cloning this repository locally, you can install and build the site as
```
apt-get install -y doxygen
pip install -r requirements.txt
mkdocs serve
```
and this will then show you the URL at which to point your browser to see your local version.