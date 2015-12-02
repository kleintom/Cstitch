Cstitch
-------
**Cstitch** is a free and open source program for creating cross stitch
patterns from images.

http://cstitch.sourceforge.net/

See **INSTALL_linux.md** for linux compile instructions.

### Release Notes ###
Beta version 0.9.7 released on December 2, 2015:
* Fixes for a couple of issues causing projects to fail to open. **All Cstitch
  users should update to this release to avoid loss of data due to these
  bugs!**
    * Older versions of Cstitch have a bug related to deleting images which can
      cause a square image that was never deleted to not be saved in a project,
      even though that square image was visible on screen at the time of the
      save.  Unfortunately the data associated with that square image is gone
      for good.  If you believe you've experienced this issue (maybe Cstitch
      crashed when you loaded your project file the next time), your project
      should load with the new version of Cstitch, but it may warn you that it
      has detected an error.  If resaving your project with the new version of
      Cstitch doesn't cause that error to go away, feel free to send me your
      project file and I may be able to help (though unfortunately I can't
      retrieve any data that has been lost in this case).
    * Older versions of Cstitch will refuse to open very large project files
      (around 15,000 history edits seems to be about enough).  If you think
      you may have run into this issue, loading your project file with this
      new version of Cstitch should work.
* Added shortcut keys to Undo an edit (Ctrl+z) and Redo an edit (Ctrl+y).

Beta version 0.9.6 released on January 24, 2015:
* Better support for images with transparency
* Three additions contributed by [Simon Norberg](https://github.com/Norberg):
    * Updated colors and number of flosses on the DMC floss list to match
      [KXStitch](http://sourceforge.net/projects/kxstitch/) and
      [Crosti](http://sourceforge.net/projects/crosti/)
    * Added support for choosing colors by DMC or Anchor floss name/number
      when editing your pattern
    * Enabled 1-1 squaring; in other words, each pixel of your original
    image becomes a square in your final pattern
* April 28, 2015 update: bug fix (only affects Linux users)

Beta version 0.9.5 released on May 5, 2014:
* Updates for internationalization
* Italian translation provided by Michele Marino - thanks Michele!  The
  Italian translation will load automatically based on your computer's
  locale settings.

Beta version 0.9.4 released on January 24, 2014:
* bug fixes
* recent images and projects menus
* ctrl-mouse wheel zooming, shift-mouse wheel horizontal scrolling

Beta version 0.9.3 released on June 20, 2011:
* bug fixes
* now support Anchor floss in addition to DMC floss

Beta version 0.9.2 released on May 8, 2011:
* bug fixes
* no longer using pattern symbols with thickened border (they were hard to distinguish)
* specify pdf symbol size on pdf save
* autoload pdf viewer on pdf save

Beta version 0.9.1 released on April 24, 2011:
* bug fixes
* linux binary now available (see README_linux for distribution availability)

Beta version 0.9.0 released on April 6, 2011.

All executables are packed using the UPX packer
(http://upx.sourceforge.net/) to make them smaller - about 40% of the
original size.  (This has no effect on the way you run the program.)

