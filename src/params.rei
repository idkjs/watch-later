open! Core;
open! Async;
open! Import;

let dbpath: Command.Param.t(string);

/** Exactly one video ID. */

let video: Command.Param.t(Video_id.t);

/** List of video IDs.  May be empty. */

let videos: Command.Param.t(list(Video_id.t));

/** Non-empty list of video IDs. */

let nonempty_videos: Command.Param.t(list(Video_id.t));

/** Non-empty list of video IDs, or a playlist containing videos.

    [`Playlist (_, true)] indicates that the videos should be removed from the playlist
    after they are successfully processed. */

let nonempty_videos_or_playlist:
  Command.Param.t(
    [ | `Videos(list(Video_id.t)) | `Playlist(Playlist_id.t, bool)],
  );
