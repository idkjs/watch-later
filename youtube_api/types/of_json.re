open! Core_kernel;
open! Import;

type t('a) = Json.t => 'a;

let return = x => Fn.const(x);
let bind = (parse, ~f, json) => f(parse(json), json);
let map = (parse, ~f, json) => f(parse(json));

include Monad.Make({
  type nonrec t('a) = t('a);

  let return = return;
  let bind = bind;
  let map = `Custom(map);
});

[@deriving sexp_of]
exception
  Of_json({
    context: list(string),
    json: Json.t,
    exn,
  });

[@deriving sexp_of]
exception
  Type_error({
    expected: string,
    got: string,
  });

let with_empty_context = (parse, json) =>
  try(parse(json)) {
  | Of_json(_) as exn => raise(exn)
  | exn => raise(Of_json({context: [], json, exn}))
  };

let with_context = (ctx, parse, json) =>
  try(parse(json)) {
  | Of_json({context, json, exn}) =>
    raise(Of_json({context: [ctx, ...context], json, exn}))
  | exn => raise(Of_json({context: [ctx], json, exn}))
  };

let assoc_exn = (pairs, key, ~if_none, ~if_one) =>
  switch (
    List.filter_map(pairs, ~f=((key', v)) =>
      if (String.equal(key, key')) {
        Some(v);
      } else {
        None;
      }
    )
  ) {
  | [] => if_none(key)
  | [json] => if_one(json)
  | [_, _, ..._] as values =>
    raise_s(
      [%message
        "Duplicate key in object"(key: string, values: list(Json.t))
      ],
    )
  };

let raise_type_error = (~expected, ~got) =>
  raise(Type_error({expected, got}));

let expect_assoc_exn = json => {
  let raise_type_error = got => raise_type_error(~expected="assoc", ~got);
  switch (json) {
  | `Bool(_) => raise_type_error("bool")
  | `Null => raise_type_error("null")
  | `Assoc(assoc) => assoc
  | `List(_) => raise_type_error("list")
  | `Float(_) => raise_type_error("float")
  | `String(_) => raise_type_error("string")
  | `Int(_) => raise_type_error("int")
  };
};

let (@.) = (field, parse, json) => {
  let assoc = expect_assoc_exn(json);
  assoc_exn(
    assoc,
    field,
    ~if_none=key => raise_s([%message "Missing key in object"(key: string)]),
    ~if_one=member => with_context(field, parse, member),
  );
};

let (@.?) = (field, parse, json) => {
  let assoc = expect_assoc_exn(json);
  assoc_exn(
    assoc,
    field,
    ~if_none=_ => None,
    ~if_one=member => with_context(field, parse, member) |> Some,
  );
};

let null = json => {
  let raise_type_error = got => raise_type_error(~expected="null", ~got);
  switch (json) {
  | `Bool(_) => raise_type_error("bool")
  | `Null => ()
  | `Assoc(_) => raise_type_error("assoc")
  | `List(_) => raise_type_error("list")
  | `Float(_) => raise_type_error("float")
  | `String(_) => raise_type_error("string")
  | `Int(_) => raise_type_error("int")
  };
};

let null = with_empty_context(null);

let bool = json => {
  let raise_type_error = got => raise_type_error(~expected="bool", ~got);
  switch (json) {
  | `Bool(bool) => bool
  | `Null => raise_type_error("null")
  | `Assoc(_) => raise_type_error("assoc")
  | `List(_) => raise_type_error("list")
  | `Float(_) => raise_type_error("float")
  | `String(_) => raise_type_error("string")
  | `Int(_) => raise_type_error("int")
  };
};

let bool = with_empty_context(bool);

let string = json => {
  let raise_type_error = got => raise_type_error(~expected="string", ~got);
  switch (json) {
  | `Bool(_) => raise_type_error("bool")
  | `Null => raise_type_error("null")
  | `Assoc(_) => raise_type_error("assoc")
  | `List(_) => raise_type_error("list")
  | `Float(_) => raise_type_error("float")
  | `String(string) => string
  | `Int(_) => raise_type_error("int")
  };
};

let string = with_empty_context(string);

let int = json => {
  let raise_type_error = got => raise_type_error(~expected="int", ~got);
  switch (json) {
  | `Bool(_) => raise_type_error("bool")
  | `Null => raise_type_error("null")
  | `Assoc(_) => raise_type_error("assoc")
  | `List(_) => raise_type_error("list")
  | `Float(_) => raise_type_error("float")
  | `String(_) => raise_type_error("string")
  | `Int(int) => int
  };
};

let int = with_empty_context(int);

let float = json => {
  let raise_type_error = got => raise_type_error(~expected="float", ~got);
  switch (json) {
  | `Bool(_) => raise_type_error("bool")
  | `Null => raise_type_error("null")
  | `Assoc(_) => raise_type_error("assoc")
  | `List(_) => raise_type_error("list")
  | `Float(float) => float
  | `String(_) => raise_type_error("string")
  | `Int(_) => raise_type_error("int")
  };
};

let float = with_empty_context(float);

let expect_list_exn = json => {
  let raise_type_error = got => raise_type_error(~expected="list", ~got);
  switch (json) {
  | `Bool(_) => raise_type_error("bool")
  | `Null => raise_type_error("null")
  | `Assoc(_) => raise_type_error("assoc")
  | `List(values) => values
  | `Float(_) => raise_type_error("float")
  | `String(_) => raise_type_error("string")
  | `Int(_) => raise_type_error("int")
  };
};

let list = (parse, json) => {
  let values = expect_list_exn(json);
  List.map(values, ~f=parse);
};

let list = parse => with_empty_context(list(parse));
let json = Fn.id;
let lazy_ = (parse, json) => lazy(parse(json));

module Let_syntax = {
  include Let_syntax;

  let null = null;
  let bool = bool;
  let string = string;
  let int = int;
  let float = float;
  let list = list;
  let (@.) = (@.);
  let (@.?) = (@.?);
  let lazy_ = lazy_;

  module Let_syntax = {
    include Let_syntax;

    module Open_on_rhs = {
      include Monad_infix;

      let return = return;
      let null = null;
      let bool = bool;
      let string = string;
      let int = int;
      let float = float;
      let list = list;
      let (@.) = (@.);
      let (@.?) = (@.?);
      let lazy_ = lazy_;
    };
  };
};

let run = (json, parse) =>
  try(Ok(with_empty_context(parse, json))) {
  | Of_json({context, json, exn}) =>
    Or_error.error_s(
      [%message
        "Failed to parse JSON"(context: list(string), json: Json.t, exn: exn)
      ],
    )
  };

let run_exn = (json, parse) => run(json, parse) |> Or_error.ok_exn;
