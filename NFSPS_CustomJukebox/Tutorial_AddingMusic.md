# Music Adding Tutorial

This is a basic guide on how to add (not replace) music tracks to NFS Pro Street.

## Required tools

- [MPFmaster](https://github.com/xan1242/MPFmaster)

- [snrtool](https://github.com/xan1242/MPFmaster/tree/master/snrtool) (get the entire snrtool_bin folder)

- And finally, this plugin itself for NFS Pro Street ([NFSPS_CustomJukebox](https://github.com/xan1242/NFS_CustomJukebox/releases))

## Step 1: Extracting & decompiling

- Locate the Pathfinder data files in SOUND\PFDATA (ps_music.mus and ps_music.mpf)

- Copy the files somewhere else where you can edit them (let's call it the EDIT folder)

- To that same EDIT folder, extract and copy the MPFmaster tool

- Open a command prompt/terminal in the EDIT folder

- Decompile the map file with mpfmaster: `mpfmaster ps_music.mpf`

- Extract all samples with mpfmaster: `mpfmaster -sa ps_music.mpf ps_music.mus ps_music_samples`

## Step 2: Adding an EventID & Sample to the map file

- With mpfmaster, append a new slot: `mpfmaster -ap ps_music_decomp.txt`

- The output will be something like this:

```
EA Pathfinder v5 MPF tool
Appending EventID: [0x5E11E], Sample: [2684]
```

- Take note of the EventID and Sample numbers

## Step 3: Encoding the sample(s)

- With snrtool, encode an audio sample from a .wav file: `snrtool audio.wav` (you can drag and drop a wav file to the tool)

- snrtool will output 2 files: `audio.sns` and `audio.snr`

- Rename those files to: `2684.sns` and `2684.snr` respectively

- Copy those files into the ps_music_samples folder found within the EDIT folder

## Step 4: Rebuilding the Pathfinder data

- With mpfmaster, use the compilation command like this: `mpfmaster -c ps_music_decomp.txt ps_music_added.mpf`

- Update samples: `mpfmaster -su ps_music_added.mpf ps_music_samples`

## Step 5: Copying the data to the game

- If you hadn't already, back up your original ps_music.mpf & mus from SOUND\PFDATA in the game

- Copy your newly created files from the EDIT folder back into the SOUND\PFDATA using the same names as the original files (ps_music_added.mpf & mus become ps_music.mpf & mus)

## Step 6: Adding the track to the game playlist

Now that the data is actually within the files, the game still doesn't know that it's there to play them. This is where CustomJukebox comes into action.

- If you hadn't already, install NFSPS_CustomJukebox

- In the CustomPlaylists folder, create a new text document with an `.ini` extension (or open the included `StockPlaylist.ini`)

- Open the file with a text editor, and add the following contents to it:

```ini
[0x5E11E]
Name = My Song Name
Artist = My Song Artist
Album = My Song Album
Playability = 3
```

- The section name (in `[]`) is the EventID that you noted earlier.
- Make sure you're only using ANSI encoding! Any other encoding will be problematic! You can check this with text editors such as [Notepad++](https://notepad-plus-plus.org/) or Windows 11's Notepad in the status bar (bottom right). In case you're using Windows' Notepad, make sure you set the encoding in the "Save As" window correctly.
- Follow the guidelines found in the [example ini found here](https://github.com/xan1242/NFS_CustomJukebox/blob/master/NFSPS_CustomJukebox/PlaylistExample.ini) so you understand what's what.

## Step 7: Checking the playlist

- Open the EA Trax Jukebox in the game options menu to check if the song is there. Depending on the index you provided in the ini, you should see it in the list.

## Step 8: Adding even more tracks

Now that you understand how to add a single track, the steps which you need to repeat should be logical.

- Repeat steps 2 and 3 for as many tracks as you need

- Rebuild & copy the new Pathfinder data to the game

- Add those EventIDs into the playlist ini file like in Step 6

## Other notes

If you managed to follow this along, the method to replace the existing songs should be pretty straightforward to you as well. 

Simply replace the sns and snr files inside the sample folder to replace the songs with your own!

And to edit their data, simply edit the StockPlaylist.ini file in the CustomPlaylists folder.
