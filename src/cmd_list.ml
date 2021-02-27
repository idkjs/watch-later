open! Core
open! Async
open! Import

module Which_videos = struct
  type t =
    | These of Video_id.t list
    | Filter of Video_db.Filter.t
end

let main ~dbpath ~id ~(which_videos : Which_videos.t) =
  let print ((video_info : Video_info.t), watched) =
    if id
    then printf !"%{Video_id}\n" video_info.video_id
    else print_s [%message (video_info : Video_info.t) (watched : bool)]
  in
  Video_db.with_file_and_txn dbpath ~f:(fun db ->
    match which_videos with
    | These ids ->
      Deferred.Or_error.List.iter ids ~f:(fun video_id ->
        let%bind info =
          match%bind Video_db.get db video_id with
          | Some info -> return info
          | None ->
            Deferred.Or_error.error_s
              [%message "Video not found" (video_id : Video_id.t)]
        in
        print info;
        return ())
    | Filter filter ->
      let%bind () =
        Video_db.get_videos db filter
        |> Pipe.iter_without_pushback ~f:print
        |> Deferred.ok
      in
      return ())
;;

let command =
  Command.async_or_error
    ~summary:"List videos according to filter."
    (let%map_open.Command () = return ()
     and dbpath = Params.dbpath
     and video_ids = Params.videos
     and filter = Video_db.Filter.param ~default_to_unwatched:false
     and id =
       flag
         "-id"
         no_arg
         ~doc:" If passed, print just the video ID rather than all the video info"
     in
     fun () ->
       Writer.behave_nicely_in_pipeline ();
       let which_videos : Which_videos.t =
         match video_ids, Video_db.Filter.is_empty filter with
         | _ :: _, false -> failwith "Cannot specify both video IDs and filter"
         | _ :: _, true -> These video_ids
         | [], _ -> Filter filter
       in
       main ~dbpath ~id ~which_videos)
;;
