import sublime_plugin
from subprocess import Popen, PIPE, STDOUT
import os
import sublime
import copy

class GitWipAutoCommand(sublime_plugin.EventListener):

    def on_post_save_async(self, view):
        dirname, fname = os.path.split(view.file_name())

        p = Popen(["git", "wip", "save",
                   "WIP from ST3: saving %s" % fname,
                   "--editor", "--", fname],
                  cwd=dirname, universal_newlines=True,
                  bufsize=8096, stdout=PIPE, stderr=STDOUT)

        def finish_callback():
            rcode = p.poll()

            if rcode is None: # not terminated yet
                sublime.set_timeout_async(finish_callback, 20)
                return

            if rcode != 0:
                print ("git command returned code {}".format(rcode))

            for line in p.stdout:
                print(line)

        finish_callback()

