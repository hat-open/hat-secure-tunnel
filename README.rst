.. _git repository: https://github.com/hat-open/hat-iec62351-tunnel.git
.. _Hat Open: https://hat-open.com
.. _Končar Digital: https://www.koncar.hr/en


hat-iec62351-tunnel - IEC 62351-3 tunnel
========================================

Application providing tunnel between unencrypted and TLS communication
based on IEC 62351-3.

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

During ``configure``, following environment variables are checked:

* CC
* CFLAGS
* LDFLAGS
* PREFIX
* DESTDIR


Running
-------

...


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
