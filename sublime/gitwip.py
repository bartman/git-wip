# Copyright Bart Trojanowski <bart@jukie.net>
#
# This file is part of git-wip.
#
# git-wip is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# git-wip is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with git-wip. If not, see <https://www.gnu.org/licenses/>.

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

