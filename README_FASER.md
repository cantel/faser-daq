# FASER

FASER specific instructions for an overview of the DAQ system, setup/building, running,
and development can be found at - [https://faserdaq.web.cern.ch/faserdaq/](https://faserdaq.web.cern.ch/faserdaq/).

## Installation

Follow instructions in the main README.md and daqling/README.md on how to clone the repository
and setup the host machine. Please note at moment you need:
  * To install at least the build directory on a non-AFS filesystem 
  * CentOS7 machine with root access (or a host that was already setup machine)
  * All the optional ansible options in daqling should be installed except Cassandra
  * The web gui also needs the following command to run

    sudo python3 -m pip install Flask_Scss

## Documentation

The documentation that is built and deployed to [https://faserdaq.web.cern.ch/faserdaq/](https://faserdaq.web.cern.ch/faserdaq/)
is based on the example provided here [https://gitlab.cern.ch/gitlabci-examples/deploy_eos](https://gitlab.cern.ch/gitlabci-examples/deploy_eos).
In particular, the documentation is built using [mkdocs](www.mkdocs.org) housed on EOS - both of these
are executed in the [.gitlab-ci.yml](.gitlab-ci.yml) housed here.  You can learn more about GitLab Pages at https://pages.gitlab.io and the official
documentation https://docs.gitlab.com/ce/user/project/pages/.

### Building locally

To work locally with this project, you'll have to follow the steps below:

1. Fork, clone or download this project
2. Install [mkdocs](www.mkdocs.org)
3. Preview your project: `mkdocs serve`,
   your site can be accessed under `localhost:8000`
4. Add content
5. Generate the website: `mkdocs build` (optional)

Read more at MkDocs [documentation](www.mkdocs.org).

