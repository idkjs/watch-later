open! Core;
open! Async;
open! Import;

[@deriving sexp]
type contents = {
  client_id: string,
  client_secret: string,
  mutable access_token: string,
  refresh_token: string,
  mutable expiry: Time_ns.t,
};

type t = {
  file: string,
  contents,
};

let on_disk = (~file=Watch_later_directories.oauth_credentials_path, ()) => {
  let%map contents = Reader.load_sexp(file, [%of_sexp: contents]);
  {file, contents};
};

let save = t => {
  let%bind () =
    Monitor.try_with_or_error(() =>
      Unix.mkdir(~p=(), Filename.dirname(t.file))
    );

  let%bind () =
    Monitor.try_with_or_error(() =>
      Writer.save_sexp(~perm=0o600, t.file, [%sexp (t.contents: contents)])
    );

  return();
};

let of_json_save_to_disk =
    (
      ~file=Watch_later_directories.oauth_credentials_path,
      json,
      ~client_id,
      ~client_secret,
    ) => {
  let%bind (access_token, refresh_token, expiry) =
    Of_json.run(
      json,
      {
        let%map_open.Of_json () = return()
        and access_token = "access_token" @. string
        and refresh_token = "refresh_token" @. string
        and expires_in = "expires_in" @. (int >>| Time_ns.Span.of_int_sec);
        (
          access_token,
          refresh_token,
          Time_ns.add(Time_ns.now(), expires_in),
        );
      },
    )
    |> Deferred.return;

  let t = {
    file,
    contents: {
      client_id,
      client_secret,
      access_token,
      refresh_token,
      expiry,
    },
  };

  let%bind () = save(t);
  return(t);
};

let refresh_contents =
    (
      {client_id, client_secret, access_token: _, refresh_token, expiry: _} as contents,
    ) => {
  let endpoint =
    Uri.make(
      ~scheme="https",
      ~host="oauth2.googleapis.com",
      ~path="/token",
      (),
    );

  let%bind (response, body) =
    Monitor.try_with_or_error(() =>
      Cohttp_async.Client.post_form(
        endpoint,
        ~params=[
          ("client_id", [client_id]),
          ("client_secret", [client_secret]),
          ("grant_type", ["refresh_token"]),
          ("refresh_token", [refresh_token]),
        ],
      )
    );

  if (response.status |> Cohttp.Code.code_of_status |> Cohttp.Code.is_success) {
    let%bind json = Cohttp_async.Body.to_string(body) |> Deferred.ok;
    let%bind json =
      Deferred.return(Or_error.try_with(() => Json.of_string(json)));
    let%bind (access_token, expiry) =
      /* FIXME: This code is similar to [cmd_oauth]. */
      Of_json.run(
        json,
        {
          let%map_open.Of_json access_token = "access_token" @. string
          and expires_in = "expires_in" @. int >>| Time_ns.Span.of_int_sec;
          (access_token, Time_ns.add(Time_ns.now(), expires_in));
        },
      )
      |> Deferred.return;

    contents.access_token = access_token;
    contents.expiry = expiry;
    return();
  } else {
    Deferred.Or_error.error_s(
      [%message
        "Failed to refresh access token"(
          ~status=Cohttp.Code.string_of_status(response.status),
        )
      ],
    );
  };
};

let refresh = t => {
  let%bind () = refresh_contents(t.contents);
  let%bind () = save(t);
  return();
};

let expiry_delta = Time_ns.Span.of_int_sec(10);

let is_expired = t =>
  Time_ns.is_earlier(
    Time_ns.sub(t.contents.expiry, expiry_delta),
    ~than=Time_ns.now(),
  );

let access_token = t => {
  let%bind () =
    if (is_expired(t)) {
      refresh(t);
    } else {
      return();
    };
  return(t.contents.access_token);
};
