# NFS Custom Jukebox Playlists

This is a plugin which overrides the built-in jukebox playlist with a custom one.

It is made with the sole purpose of simplifying access to the jukebox playlist by removing the need to edit the Attrib database (VLT)

This does NOT replace the music data itself. To modify music data, use a tool such as [MPFmaster](https://github.com/xan1242/MPFmaster).

This is also unrelated to the [XNFSMusicPlayer](https://github.com/xan1242/xnfsmusicplayer) project. This is intended to modify the music playlist via the internal music player of the game, whereas the aformentioned project is intended to modify the music via a custom player.

## Usage

1. Create a folder somewhere within the game files (by default it's "CustomPlaylist")

2. Set that path in the main ini file (PlaylistFolder) - skip if you're using the default path

3. Create ini files with event IDs in hexadecimal format as their filenames (aka pathevent in Attrib/VLT)

4. Inside the ini, define the track properties (name, album, artist, index) - check TrackExample.ini (or 12345678.ini in the release package) for more and detailed info

## Compatibility

Currently it ONLY works for NFS Pro Street as it was built with priority (for another project).

However, as of now, the custom playlists for Most Wanted and Carbon are already figured out for another project - [XNFSMusicPlayer](https://github.com/xan1242/xnfsmusicplayer) which is basically the same thing except it also replaces the music player itself. Taking the playlist replacer out of that code will allow for this exact same functionality.

MW and Carbon are on the TODO list and are probably coming soon.

Undercover is potentially very similar.

Underground 2 is also possible, but, Pathfinder version 4 hadn't yet been fully figured out.

And lastly, Underground 1 is completely different and doesn't use Pathfinder, so this will not apply to it in the same way.
