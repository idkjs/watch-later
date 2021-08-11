open! Core;
open! Async;
open! Import;

type t = {creds: Youtube_api_oauth.Oauth.t};

let create = (~creds) => {creds: creds};

let command = (~extract_exn=?, ~summary, ~readme=?, param) =>
  Command.async_or_error(
    ~extract_exn?,
    ~summary,
    ~readme?,
    {
      let%map_open.Command () = return()
      /* TODO: add flag for oauth credentials file */
      and () = Log.Global.set_level_via_param()
      and main = param;
      () => {
        let%bind creds = Youtube_api_oauth.Oauth.on_disk();
        let api = create(~creds);
        main(api);
      };
    },
  );

let http_call_internal = (t, ~body=?, method, uri, ~headers) => {
  let max_retries = 2;
  let rec loop = tries_remaining => {
    let%bind (response, body) =
      Monitor.try_with_or_error(() =>
        Cohttp_async.Client.call(~body?, method, uri, ~headers)
      );

    if (Poly.equal(`Unauthorized, response.status)) {
      let tries_remaining = tries_remaining - 1;
      if (tries_remaining > 0) {
        %log.global.error
        "Got HTTP 401 Unauthorized, refreshing credentials and retrying"(
          tries_remaining: int,
        );
        let%bind () = Youtube_api_oauth.Oauth.refresh(t.creds);
        loop(tries_remaining);
      } else {
        Deferred.Or_error.error_s(
          [%message
            "Failed after retrying"(
              max_retries: int,
              response: Cohttp.Response.t,
            )
          ],
        );
      };
    } else {
      return((response, body));
    };
  };

  loop(max_retries);
};

let status_equal: Equal.t(Cohttp.Code.status_code) = (
  Poly.equal: Equal.t(Cohttp.Code.status_code)
);

let call = (~body=?, t, endpoint, ~method_ as method, ~params, ~expect_status) => {
  let uri = {
    let path = "youtube/v3" ^/ endpoint;
    Uri.with_query'(
      Uri.make((), ~scheme="https", ~host="www.googleapis.com", ~path),
      params,
    );
  };

  let%bind access_token = Youtube_api_oauth.Oauth.access_token(t.creds);
  let (headers, uri) = (
    Cohttp.Header.init_with("Authorization", "Bearer " ++ access_token),
    uri,
  );

  let body = Option.map(body, ~f=json => `String(Json.to_string(json)));
  %log.global.debug
  "Making YouTube API request"(
    method: Cohttp.Code.meth,
    uri: Uri_sexp.t,
    headers: Cohttp.Header.t,
    body: [@sexp.option] option(Cohttp.Body.t),
  );
  let%bind (response, body) =
    http_call_internal(t, ~body?, method, uri, ~headers);
  let%bind body = Cohttp_async.Body.to_string(body) |> Deferred.ok;
  %log.global.debug
  "Received response"(response: Cohttp.Response.t, body: string);
  if (status_equal(response.status, expect_status)) {
    return((response, body));
  } else {
    Deferred.Or_error.error_s(
      [%message
        "unacceptable status code"(
          ~status=response.status: Cohttp.Code.status_code,
          ~expected=expect_status: Cohttp.Code.status_code,
          body: string,
        )
      ],
    );
  };
};

let get = (~body=?, t, endpoint, ~params) => {
  let%bind (_response, body) =
    call(~body?, t, endpoint, ~method_=`GET, ~expect_status=`OK, ~params);

  let%bind json =
    Deferred.return(Or_error.try_with(() => Json.of_string(body)));
  return(json);
};

let exec = (~body=?, t, endpoint, ~method_ as method, ~params, ~expect_status) => {
  let%bind (_response, body) =
    call(~body?, t, endpoint, ~method_=method, ~expect_status, ~params);
  let%bind json =
    Deferred.return(Or_error.try_with(() => Json.of_string(body)));
  return(json);
};

let exec_expect_empty_body =
    (~body=?, t, endpoint, ~method_ as method, ~params, ~expect_status) => {
  let%bind (_response, body) =
    call(~body?, t, endpoint, ~method_=method, ~expect_status, ~params);
  if (String.is_empty(body)) {
    return();
  } else {
    Deferred.Or_error.error_s(
      [%message "Expected empty response body"(body: string)],
    );
  };
};
