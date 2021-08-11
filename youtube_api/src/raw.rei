/** Code common to all API endpoints. */;

open! Core;
open! Async;
open! Import;

type t;

let command:
  (
    ~extract_exn: bool=?,
    ~summary: string,
    ~readme: unit => string=?,
    Command.Param.t(t => Deferred.t(Or_error.t(unit)))
  ) =>
  Command.t;

let get:
  (~body: Json.t=?, t, string, ~params: List.Assoc.t(string, string)) =>
  Deferred.t(Or_error.t(Json.t));

let exec:
  (
    ~body: Json.t=?,
    t,
    string,
    ~method_: Cohttp.Code.meth,
    ~params: List.Assoc.t(string, string),
    ~expect_status: Cohttp.Code.status_code
  ) =>
  Deferred.t(Or_error.t(Json.t));

let exec_expect_empty_body:
  (
    ~body: Json.t=?,
    t,
    string,
    ~method_: Cohttp.Code.meth,
    ~params: List.Assoc.t(string, string),
    ~expect_status: Cohttp.Code.status_code
  ) =>
  Deferred.t(Or_error.t(unit));
