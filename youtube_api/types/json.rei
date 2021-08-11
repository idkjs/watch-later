open! Core_kernel;
open! Import;

/** JSON values.

    [sexp_of_t] is just an atom of the JSON string. */

[@deriving sexp_of]
type t = Yojson.Basic.t;

include Stringable with type t := t;
include Pretty_printer.S with type t := t;

let to_string_pretty: t => string;
