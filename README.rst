.. _git repository: https://github.com/hat-open/hat-iec62351-tunnel.git
.. _Hat Open: https://hat-open.com
.. _Končar Digital: https://www.koncar.hr/en


hat-iec62351-tunnel - IEC 62351-3 tunnel
========================================

Application providing tunnel between unencrypted and TLS communication
based on IEC 62351-3.

`hat-iec62351-tunnel` listens for incoming connections. Once incoming TCP
connection is established, `hat-iec62351-tunnel` creates new outgoing connection
to other TCP server. These two connections are paired and all data received
on one connection is sent to other. Closing of one connection results in closing
of other connection. Both incoming and outgoing connections can be configured to
use TLS as defined by IEC 62351-3.

* `git repository`_


Dependencies
------------

* jansson
* libuv
* openssl


Build and install
-----------------

::

    $ ./configure
    $ make
    $ make install


Running
-------

`hat-iec62351-tunnel` accepts path to JSON configuration as command line
argument::

    Usage: hat-iec62351-tunnel [-h] [-l LOG_LEVEL] <conf_path>

      Arguments:
        <conf_path>   Path to configuration file or - for stdin

      Options:
        -h            Display usage help and exit
        -l LOG_LEVEL  Set logging level (DEBUG, INFO, WARNING, ERROR)
                      (default INFO)

Configuration structure is defined by JSON Schema
``hat-iec62351-tunnel://main.yaml`` available as part of `hat-iec62351-tunnel`
manual page::

    $ man hat-iec62351-tunnel


Hat Open
--------

`hat-iec62351-tunnel` is part of `Hat Open`_ project - open-source framework of
tools and libraries for developing applications used for remote monitoring,
control and management of intelligent electronic devices such as IoT devices,
PLCs, industrial automation or home automation systems.

Development of Hat Open and associated repositories is sponsored by
`Končar Digital`_.


License
-------

Copyright 2020-2026 Hat Open AUTHORS

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
