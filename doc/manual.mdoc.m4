.Dd February 6, 2026
.Dt _TITLE 1
.Os

.Sh NAME
.Nm _NAME
.Nd IEC 62351-3 tunnel

.Sh SYNOPSIS
.Nm
.Op Fl h
.Op Fl l Ar LOG_LEVEL
.Ar <conf-path>

.Sh DESCRIPTION
.Nm
provides tunnel between unencrypted and TLS communication based on IEC 62351-3.

Arguments:
.Bl -tag

.It Fl h
Display usage help and exit

.It Fl l Ar LOG_LEVEL
Set logging level (DEBUG, INFO, WARNING, ERROR) (default INFO)

.It Ar <conf-path>
Path to configuration file or - for stdin

.El

.Ss ABOUT

.Nm
listens for incoming connections. Once incoming TCP connection is established,
.Nm
creates new outgoing connection to other TCP server. These two connections are
paired and all data received on one connection is sent to other. Closing of one
connection results in closing of other connection. Both incoming and outgoing
connections can be configured to use TLS as defined by IEC 62351-3.

.Ss CONFIGURATION

Structure of configuration file is defined by
.Sx JSON Schema configuration definition

Single configuration can include multiple
.Em mapping
parts. Each
.Em mapping
represents single listening socket that can accept multiple incoming
server connections. Listening socket and incoming connections are configured
with
.Em server
property, while associated outgoing connections are configured with
.Em client
property.

In case of server endpoint,
.Em host
represents listening lost name or IP address, while in case of client endpoint,
.Em host
represents remote host name or IP address.

If
.Em ssl
property is ommited, TLS communication is not enabled.

.Em local_cert
property provides paths to files containing local certificate and private key.
This property can be omitted in case of client endpoint. By omitting this
property, client connection will not include client side sertificate.
Content of
.Em cert_path
and
.Em key_path
is read during application startup. Optional
.Em password
property is used if certificate/key is additionally protected with password.

.Em remote_cert
property provides information for remote certificate validation. If this
property is omitted, remote certificate validation is not preformed.

.Em ca_path
property references certificate authority certificate responsible for signing
peer certificate. This file can contain multiple CA certificates. It is
recommended to configure at least two different trust anchors (for example, by
including at least two distinct root CA certificates in the file referenced by
.Em ca_path Ns ).

If
.Em allowed_cert_sn
is provided, only certificates with serial number explicitly configured by
.Em allowed_cert_sn
are permitted.

.Em max_cert_len
property defines maximum allowed peer certificate length.

.Em crl
property enables certificate revocation list checks. Path to CRL is read
during connection establishment and optionally updated if
.Em update_interval
is specified. If
.Em update_interval
is not provided, CRL is read only during connection establishment regardless
of CRL's nextUpdate value.

.Em renegotiate_interval
property specifies optional TLS renegotiation interval. If renegotiation is
initiated by peer, timer responsible for triggering next renegotiation is reset.
TLS session renegotiation is controlled by the
.Em renegotiate_interval
parameter. It is recommended to configure this interval to no less than 600
seconds (10 minutes) and no less than half of the configured CRL update period.
Using shorter values may reduce alignment with the timing expectations defined
in IEC 62351-4.

.Em session_cache_timeout
property enables specification of session duration in case of session
resumption. TLS session resumption is supported through the use of session
identifiers and session tickets, with tickets being preferred. It is
recommended to configure the system so that session resumption remains possible
at least every two hours, for example by setting
.Em session_cache_timeout
to 7200 seconds or higher. Session state is maintained in memory only,
therefore session resumption is available only while the process is running.
After a process restart, a new full TLS handshake is required before resumption
can be used again.

.Ss JSON Schema configuration definition

.Bd -literal

include(_JSON_SCHEMA_PATH)

.Ed

.Sh EXIT STATUS
.Ex -std

.Sh AUTHORS
.An Hat Open AUTHORS
