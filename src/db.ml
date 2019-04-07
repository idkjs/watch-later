open! Core
open! Async
open! Import

module Reader = struct
  module T = struct
    type 'a t = Sqlite3.Data.t array -> Sqlite3.Data.t String.Map.t -> 'a

    let return x _ _ = x
    let map = `Custom (fun t ~f data by_headers -> f (t data by_headers))

    let apply tf
          tx
          data
          by_headers =
      let f = tf data by_headers in
      let x = tx data by_headers in
      f x
    ;;
  end

  module For_let_syntax = struct
    include T
    include Applicative.Make (T)
  end

  include For_let_syntax

  let by_index index
        data
        _by_headers = data.(index)

  let by_name name
        _data
        by_headers = Map.find_exn by_headers name

  module Open_on_rhs_intf = struct
    module type S = sig
      val by_index : int -> Sqlite3.Data.t t
      val by_name : string -> Sqlite3.Data.t t
    end
  end

  include Applicative.Make_let_syntax (For_let_syntax) (Open_on_rhs_intf)
      (struct
        let by_index = by_index
        let by_name = by_name
      end)

  let stmt t stmt =
    let data = Sqlite3.row_data stmt in
    let headers = Sqlite3.row_names stmt in
    let by_headers =
      Array.map2_exn data headers ~f:(fun data header -> header, data)
      |> Array.to_list
      |> String.Map.of_alist_exn
    in
    t data by_headers
  ;;
end

(* TODO: More informative error messages. *)
(* TODO: Arity_n *)
(* TODO: Separate Sqlite3_with_gadts library. *)
module Arity = struct
  type t0 = unit Deferred.t
  type t1 = Sqlite3.Data.t -> t0
  type t2 = Sqlite3.Data.t -> t1
  type t3 = Sqlite3.Data.t -> t2
  type t4 = Sqlite3.Data.t -> t3

  type 'f t =
    | Arity0 : t0 t
    | Arity1 : t1 t
    | Arity2 : t2 t
    | Arity3 : t3 t
    | Arity4 : t4 t

  let to_int (type a) : a t -> int = function
    | Arity0 -> 0
    | Arity1 -> 1
    | Arity2 -> 2
    | Arity3 -> 3
    | Arity4 -> 4
  ;;
end

module Kind = struct
  type 'kind t =
    | Select : [ `Select ] t
    | Non_select : [ `Non_select ] t
end

module Stmt = struct
  type ('kind, 'input_callback) stmt =
    | Select :
        Sqlite3.stmt * 'input_callback Arity.t
        -> ([ `Select ], 'input_callback) stmt
    | Non_select :
        Sqlite3.stmt * 'input_callback Arity.t
        -> ([ `Non_select ], 'input_callback) stmt

  type ('kind, 'input_callback) t =
    { stmt : ('kind, 'input_callback) stmt
    ; thread : In_thread.Helper_thread.t
    }

  let reset stmt ~thread =
    match%map In_thread.run ~thread (fun () -> Sqlite3.reset stmt) with
    | OK -> ()
    | rc -> failwithf !"unexpected return code: %{Sqlite3.Rc}" rc ()
  ;;

  let bind_exn stmt index data =
    match Sqlite3.bind stmt index data with
    | OK -> ()
    | rc -> failwithf !"unexpected return code: %{Sqlite3.Rc}" rc ()
  ;;

  let select_exn
        (type i)
        { stmt = Select (stmt, (input_arity : i Arity.t)); thread }
        reader
        ~f
    : i
    =
    let rec loop () =
      match%bind In_thread.run ~thread (fun () -> Sqlite3.step stmt) with
      | ROW ->
        let x = Reader.stmt reader stmt in
        f x;
        loop ()
      | DONE -> return ()
      | rc -> failwithf !"unexpected return code: %{Sqlite3.Rc}" rc ()
    in
    let go () =
      let%bind () = reset stmt ~thread in
      loop ()
    in
    match input_arity with
    | Arity0 -> go ()
    | Arity1 ->
      fun a ->
        bind_exn stmt 1 a;
        go ()
    | Arity2 ->
      fun a b ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        go ()
    | Arity3 ->
      fun a b c ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        bind_exn stmt 3 c;
        go ()
    | Arity4 ->
      fun a b c d ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        bind_exn stmt 3 c;
        bind_exn stmt 4 d;
        go ()
  ;;

  let select_exn'
        (type i)
        { stmt = Select (stmt, (input_arity : i Arity.t)); thread }
        reader
        ~f
    : i
    =
    let rec loop () =
      match%bind In_thread.run ~thread (fun () -> Sqlite3.step stmt) with
      | ROW ->
        let x = Reader.stmt reader stmt in
        let%bind () = f x in
        loop ()
      | DONE -> return ()
      | rc -> failwithf !"unexpected return code: %{Sqlite3.Rc}" rc ()
    in
    let go () =
      let%bind () = reset stmt ~thread in
      loop ()
    in
    match input_arity with
    | Arity0 -> go ()
    | Arity1 ->
      fun a ->
        bind_exn stmt 1 a;
        go ()
    | Arity2 ->
      fun a b ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        go ()
    | Arity3 ->
      fun a b c ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        bind_exn stmt 3 c;
        go ()
    | Arity4 ->
      fun a b c d ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        bind_exn stmt 3 c;
        bind_exn stmt 4 d;
        go ()
  ;;

  let run_exn (type a) { stmt = Non_select (stmt, (arity : a Arity.t)); thread } : a =
    let go () =
      let%bind () = reset stmt ~thread in
      match%map In_thread.run ~thread (fun () -> Sqlite3.step stmt) with
      | DONE -> ()
      | rc -> failwithf !"unexpected return code: %{Sqlite3.Rc}" rc ()
    in
    match arity with
    | Arity0 -> go ()
    | Arity1 ->
      fun a ->
        bind_exn stmt 1 a;
        go ()
    | Arity2 ->
      fun a b ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        go ()
    | Arity3 ->
      fun a b c ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        bind_exn stmt 3 c;
        go ()
    | Arity4 ->
      fun a b c d ->
        bind_exn stmt 1 a;
        bind_exn stmt 2 b;
        bind_exn stmt 3 c;
        bind_exn stmt 4 d;
        go ()
  ;;
end

type t =
  { db : Sqlite3.db
  ; thread : In_thread.Helper_thread.t
  }

let open_file file =
  let%bind thread = In_thread.Helper_thread.create () ~name:"Watch_later.Db" in
  let%bind db = In_thread.run ~thread (fun () -> Sqlite3.db_open file) in
  return { db; thread }
;;

let rec close t =
  if Sqlite3.db_close t.db
  then return ()
  else (
    let%bind () = Clock_ns.after (Time_ns.Span.of_sec 0.05) in
    close t)
;;

let with_file dbpath ~f =
  let%bind t = open_file dbpath in
  Monitor.protect (fun () -> f t) ~finally:(fun () -> close t)
;;

let prepare_exn (type k i)
      t
      (kind : k Kind.t)
      (input_arity : i Arity.t)
      sql
  : (k, i) Stmt.t
  =
  let stmt = Sqlite3.prepare t.db sql in
  let actual_input_arity = Sqlite3.bind_parameter_count stmt in
  [%test_result: int]
    ~message:"input arity mismatch"
    actual_input_arity
    ~expect:(Arity.to_int input_arity);
  let stmt : (k, i) Stmt.stmt =
    match kind with
    | Select -> Select (stmt, input_arity)
    | Non_select -> Non_select (stmt, input_arity)
  in
  { stmt; thread = t.thread }
;;
