open! Core;
open! Async;
open! Import;
include Caqti_type;

let stringable = (type a, module M: Stringable with type t = a): t(a) =>
  M.(
    custom(
      string,
      ~encode=t => Ok(to_string(t)),
      ~decode=
        s =>
          Or_error.try_with(() => of_string(s))
          |> Result.map_error(~f=Error.to_string_hum),
    )
  );

let video_id = stringable((module Video_id));

module Record = {
  type caqti_type('a) = t('a);

  type t('fields, 'fields_rest, 'a) = {
    unwrap: 'fields => 'fields_rest,
    encode: ('a => 'fields_rest, 'a) => 'fields,
    caqti_type: caqti_type('fields_rest) => caqti_type('fields),
  };

  let finish =
      (
        type a,
        type fields,
        (
          decode: fields => a,
          {unwrap: _, encode, caqti_type}: t(fields, unit, a),
        ),
      )
      : caqti_type(a) => {
    let encode = encode(Fn.const());
    let caqti_type = caqti_type(unit);
    custom(
      caqti_type,
      ~encode=x => Ok(encode(x)),
      ~decode=x => Ok(decode(x)),
    );
  };

  let init = (type fields): t(fields, fields, _) => {
    unwrap: Fn.id,
    encode: Fn.id,
    caqti_type: Fn.id,
  };

  let step =
      (
        type fields,
        type fields_rest,
        type a,
        type this_field,
        type_: caqti_type(this_field),
        field: Fieldslib.Field.t(a, this_field),
        {unwrap, encode, caqti_type}:
          t(fields, (this_field, fields_rest), a),
      )
      : (fields => this_field, t(fields, fields_rest, a)) => {
    let encode = encode_rest =>
      encode(record =>
        (Fieldslib.Field.get(field, record), encode_rest(record))
      );

    let caqti_type = rest => caqti_type(tup2(type_, rest));
    (fst << unwrap, {unwrap: snd << unwrap, encode, caqti_type});
  };
};

let video_info =
  Video_info.Fields.make_creator(
    Record.init,
    ~channel_id=Record.step(string),
    ~channel_title=Record.step(string),
    ~video_id=Record.step(video_id),
    ~video_title=Record.step(string),
  )
  |> Record.finish;

module Std = {
  include Std;

  let video_id = video_id;
  let video_info = video_info;
};
