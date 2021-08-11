open! Core;
open! Async;
open! Import;

/** [t] is an on-disk token source. */

type t;

let on_disk:
  (
    ~file: /** Defaults to [Watch_later_directories.oauth_credentials_path] */ string
             =?,
    unit
  ) =>
  Deferred.t(Or_error.t(t));

let of_json_save_to_disk:
  (
    ~file: /** Defaults to [Watch_later_directories.oauth_credentials_path] */ string
             =?,
    Json.t,
    ~client_id: string,
    ~client_secret: string
  ) =>
  Deferred.t(Or_error.t(t));

let access_token: t => Deferred.t(Or_error.t(string));
let is_expired: t => bool;
let refresh: t => Deferred.t(Or_error.t(unit));
