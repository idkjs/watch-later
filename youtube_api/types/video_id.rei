open! Core_kernel;
open! Import;
include String_id.S;

module Plain_or_in_url: {
  let of_string: string => t;
  let arg_type: Command.Arg_type.t(t);
};

include Of_jsonable.S with type t := t;
