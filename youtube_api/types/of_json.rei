/** Reader monad for JSON consumers. */;

/* FIXME: Refactor this interface and expose an openable [O] module. */

open! Core_kernel;
open! Import;

type t('a);

include Monad.S with type t('a) := t('a);

module Let_syntax: {
  let return: 'a => t('a);

  include Monad.Infix with type t('a) := t('a);

  let null: t(unit);
  let bool: t(bool);
  let string: t(string);
  let int: t(int);
  let float: t(float);
  let list: t('a) => t(list('a));
  let (@.): (string, t('a)) => t('a);
  let (@.?): (string, t('a)) => t(option('a));
  let lazy_: t('a) => t(Lazy.t('a));

  module Let_syntax: {
    let return: 'a => t('a);
    let bind: (t('a), ~f: 'a => t('b)) => t('b);
    let map: (t('a), ~f: 'a => 'b) => t('b);
    let both: (t('a), t('b)) => t(('a, 'b));

    module Open_on_rhs: {
      let return: 'a => t('a);

      include Monad.Infix with type t('a) := t('a);

      let null: t(unit);
      let bool: t(bool);
      let string: t(string);
      let int: t(int);
      let float: t(float);
      let list: t('a) => t(list('a));
      let (@.): (string, t('a)) => t('a);
      let (@.?): (string, t('a)) => t(option('a));
      let lazy_: t('a) => t(Lazy.t('a));
    };
  };
};

/** {2 Parsing primitives} */;

let null: t(unit);
let bool: t(bool);
let string: t(string);
let int: t(int);
let float: t(float);
let list: t('a) => t(list('a));

/** Identity */

let json: t(Json.t);

/** {2 Accessing objects} */;

/** [(name @. of_json) json] applies [of_json] to the member of JSON object [json] named
    [name]. */

let (@.): (string, t('a)) => t('a);

/** [(name @.? of_json) json] applies [of_json] to the member of JSON object [json] named
    [name].  Returns [None] if the field does not exist in the object. */

let (@.?): (string, t('a)) => t(option('a));

/** [(lazy of_json) json] applies [of_json] lazily. */

let lazy_: t('a) => t(Lazy.t('a));

let run: (Json.t, t('a)) => Or_error.t('a);
let run_exn: (Json.t, t('a)) => 'a;
