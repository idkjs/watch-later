open! Core;
open! Async;
open! Import;

let rec fill_random_bytes =
        (rng: Cryptokit.Random.rng, bytes, ~pos, ~len, ~byte_is_acceptable) =>
  if (len <= 0) {
    ();
  } else {
    rng#random_bytes(bytes, pos, len);
    let rec keep_acceptable_bytes = (~src_pos, ~dst_pos) =>
      if (src_pos >= Bytes.length(bytes)) {
        fill_random_bytes(
          rng,
          bytes,
          ~pos=dst_pos,
          ~len=len - (dst_pos - pos),
          ~byte_is_acceptable,
        );
      } else if (byte_is_acceptable(Bytes.get(bytes, src_pos))) {
        Bytes.set(bytes, dst_pos, Bytes.get(bytes, src_pos));
        keep_acceptable_bytes(~src_pos=src_pos + 1, ~dst_pos=dst_pos + 1);
      } else {
        keep_acceptable_bytes(~src_pos=src_pos + 1, ~dst_pos);
      };

    keep_acceptable_bytes(~src_pos=pos, ~dst_pos=pos);
  };

let is_valid_code_verifier_char =
  fun
  | 'A' .. 'Z' => true
  | 'a' .. 'z' => true
  | '0' .. '9' => true
  | '-'
  | '.'
  | '_'
  | '~' => true
  | _ => false;

let generate_code_verifier_and_challenge = {
  let rng =
    lazy(
      Cryptokit.Random.pseudo_rng(
        Cryptokit.Random.string(Cryptokit.Random.secure_rng, 20),
      )
    );

  let to_base64url =
    String.tr_multi(~target="+/", ~replacement="-_") |> unstage;
  () => {
    let buf = Bytes.create(128);
    fill_random_bytes(
      force(rng),
      buf,
      ~pos=0,
      ~len=128,
      ~byte_is_acceptable=is_valid_code_verifier_char,
    );
    let verifier =
      Bytes.unsafe_to_string(~no_mutation_while_string_reachable=buf);
    let challenge =
      verifier
      |> Cryptokit.hash_string(Cryptokit.Hash.sha256())
      |> Cryptokit.transform_string(Cryptokit.Base64.encode_compact())
      |> to_base64url;

    (verifier, challenge);
  };
};

/* TODO: Move this logic into oauth.ml */
/* Based on https://developers.google.com/youtube/v3/guides/auth/installed-apps */
let obtain_access_token_and_save = (~client_id, ~client_secret) => {
  let (code_verifier, code_challenge) =
    generate_code_verifier_and_challenge();
  let endpoint =
    Uri.make(
      ~scheme="https",
      ~host="accounts.google.com",
      ~path="/o/oauth2/v2/auth",
      ~query=[
        ("client_id", [client_id]),
        ("redirect_uri", ["urn:ietf:wg:oauth:2.0:oob"]), /* Manual copy/paste */
        ("response_type", ["code"]),
        ("scope", ["https://www.googleapis.com/auth/youtube.force-ssl"]),
        ("code_challenge", [code_challenge]),
        ("code_challenge_method", ["S256"]),
      ],
      (),
    );

  let%bind () = Browse.url(endpoint);
  let%bind authorization_code =
    Monitor.try_with_or_error(() =>
      Async_interactive.ask_dispatch_gen("Authorization Code", ~f=code =>
        if (String.is_empty(code)) {
          Error("Empty code");
        } else {
          Ok(code);
        }
      )
    );

  let token_endpoint =
    Uri.make(
      ~scheme="https",
      ~host="oauth2.googleapis.com",
      ~path="/token",
      (),
    );

  let%bind (response, body) =
    Monitor.try_with_or_error(() =>
      Cohttp_async.Client.post_form(
        token_endpoint,
        ~params=[
          ("client_id", [client_id]),
          ("client_secret", [client_secret]),
          ("code", [authorization_code]),
          ("code_verifier", [code_verifier]),
          ("grant_type", ["authorization_code"]),
          ("redirect_uri", ["urn:ietf:wg:oauth:2.0:oob"]),
        ],
      )
    );

  if (response.status |> Cohttp.Code.code_of_status |> Cohttp.Code.is_success) {
    let%bind body = Cohttp_async.Body.to_string(body) |> Deferred.ok;
    let%bind json =
      Or_error.try_with(() => Json.of_string(body)) |> Deferred.return;

    /* FIXME: Pass in [?file] arg. */
    let%bind _: Youtube_api_oauth.Oauth.t =
      Youtube_api_oauth.Oauth.of_json_save_to_disk(
        json,
        ~client_id,
        ~client_secret,
      );

    return();
  } else {
    raise_s(
      [%message
        "Failed to obtain access token"(
          ~status=response.status: Cohttp.Code.status_code,
        )
      ],
    );
  };
};

module Obtain = {
  let command =
    Command.async_or_error(
      ~summary=
        "Generate and save valid OAuth 2.0 credentials for YouTube Data API",
      {
        let%map_open.Command () = return()
        and client_id =
          flag("-client-id", required(string), ~doc="STRING OAuth Client ID")
        and client_secret =
          flag(
            "-client-secret",
            required(string),
            ~doc="STRING OAuth Client Secret",
          );

        () => obtain_access_token_and_save(~client_id, ~client_secret);
      },
    );
};

module Refresh = {
  let command =
    Command.async_or_error(
      ~summary="Obtain a fresh access token from the saved refresh token",
      {
        let%map_open.Command () = return()
        and force =
          flag(
            "-force",
            no_arg,
            ~doc=
              " Refresh access token even if it doesn't appear to have expired",
          );

        () => {
          /* FIXME: [?file] arg */
          let%bind creds = Youtube_api_oauth.Oauth.on_disk();
          if (force || Youtube_api_oauth.Oauth.is_expired(creds)) {
            Youtube_api_oauth.Oauth.refresh(creds);
          } else {
            return();
          };
        };
      },
    );
};

let command =
  Command.group(
    ~summary="Manage OAuth 2.0 credentials for YouTube Data API",
    [("obtain", Obtain.command), ("refresh", Refresh.command)],
  );
