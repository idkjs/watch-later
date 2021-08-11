open! Core_kernel;
open! Import;

let validate = id =>
  if (String.length(id) != 11) {
    error_s(
      [%message
        "Video ID had unexpected length"(
          ~expected=11,
          ~actual=String.length(id): int,
        )
      ],
    );
  } else {
    switch (
      String.find(
        id,
        ~f=
          fun
          | 'A' .. 'Z'
          | 'a' .. 'z'
          | '0' .. '9'
          | '-'
          | '_' => false
          | _ => true,
      )
    ) {
    | None => Ok()
    | Some(char) =>
      error_s([%message "Invalid character in video ID"(char: char)])
    };
  };

include String_id.Make_with_validate(
          {
            let module_name = "Watch_later.Video_id";
            let validate = validate;
          },
          {},
        );

module Plain_or_in_url = {
  let of_url = uri =>
    switch (List.Assoc.find(Uri.query(uri), "v", ~equal=String.equal)) {
    | Some([video_id]) => of_string(video_id)
    | Some(list) =>
      raise_s([%message "invalid v= query parameter"(list: list(string))])
    | None =>
      if ([%equal: option(string)](Uri.host(uri), Some("youtu.be"))) {
        of_string(Uri.path(uri) |> String.chop_prefix_exn(~prefix="/"));
      } else {
        raise_s(
          [%message "missing v= query parameter"(~url=Uri.to_string(uri))],
        );
      }
    };

  let of_string = string =>
    try(string |> Uri.of_string |> of_url) {
    | _ => of_string(string)
    };

  let arg_type = Command.Arg_type.create(of_string);
};

let of_json = Of_json.string |> Of_json.map(~f=of_string);
