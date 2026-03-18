;; Copyright Bart Trojanowski <bart@jukie.net>
;;
;; This file is part of git-wip.
;;
;; git-wip is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 2 of the License, or
;; (at your option) any later version.
;;
;; git-wip is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
;; General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with git-wip. If not, see <https://www.gnu.org/licenses/>.

(defun git-wip-wrapper () 
  (interactive)
  (let ((file-arg (shell-quote-argument (buffer-file-name))))
    (shell-command (concat "git-wip save \"WIP from emacs: " (buffer-file-name) "\" --editor -- " file-arg))
    (message (concat "Wrote and git-wip'd " (buffer-file-name)))))

(defun git-wip-if-git ()
  (interactive)
  (when (string= (vc-backend (buffer-file-name)) "Git")
    (git-wip-wrapper)))

(add-hook 'after-save-hook 'git-wip-if-git)
