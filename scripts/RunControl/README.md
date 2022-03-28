# DAQling web interface

Web interface to control DAQling

## Getting started

### Prerequisites

* Functional DAQling demo
* Running `supervisord`
* `source cmake/setup.sh`

### Installation

    pip install daqConfig-\*.\*.\*-py3-none-any.whl

### Running server

    ./run.sh

## Configuration

### Server internal configuration

File `serverconfiguration.json`:

    displayedName: displayed name of server in page title and main graphics
    states: map of graphics for states [representing color, icon]
    callbackUri: callback address for SSO
    ALLOWED_EXTENSIONS: allowed extensions of configuration files
    timeout_for_requests: how long to wait for answer of request
    timeout_session_expiration" : duration of inactive session before expiration
    LOGOUT_URL: logout url for SSO

### Configuration of processes

The server uses DAQ control tree configuration
