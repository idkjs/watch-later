open! Core;
open! Async;
open! Import;

let url: Uri.t => Deferred.t(Or_error.t(unit));
