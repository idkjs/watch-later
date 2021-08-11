open! Core_kernel;
open! Import;

module type S = {
  type t;

  let of_json: Of_json.t(t);
};
