/** Extensions to [Caqti_type]. */;

open! Core;
open! Async;
open! Import;

include (module type of {
  include Caqti_type;
});

type caqti_type('a) := t('a);

let stringable: (module Stringable with type t = 'a) => t('a);

module Record: {
  /** Support for serializing an OCaml record in field order, in conjunction with
      [ppx_fields_conv].

      Example:

      {[
        type t = { foo : int; bar : string } [@@deriving fields]

        let caqti_type : t Caqti_type.t =
          let open Caqti_type.Std in
          let f = Caqti_type.Record.step in
          Fields.make_creator
            Caqti_type.Record.init
            ~foo:(f int)
            ~bar:(f string)
          |> Caqti_type.Record.finish
        ;;
      ]}
  */;

  /** Difference list encoding of serialization functions for each field.

      [fields_rest] represents the fields that have not yet been folded over. */

  type t('fields, 'fields_rest, 'record);

  let init: t('fields, 'fields, 'record);

  let step:
    (
      caqti_type('f),
      Fieldslib.Field.t('record, 'f),
      t('fields, ('f, 'fields_rest), 'record)
    ) =>
    ('fields => 'f, t('fields, 'fields_rest, 'record));

  let finish:
    (('fields => 'record, t('fields, unit, 'record))) => caqti_type('record);
};

module Std: {
  include (module type of {
    include Std;
  });

  let video_id: t(Video_id.t);
  let video_info: t(Video_info.t);
};
