(eval-when-compile
  (require 'cl))

(require 'vc)

(defun git-wip-after-save ()
  (interactive)
  (when (string= (vc-backend (buffer-file-name)) "Git")
    (start-process "git-wip" "*git-wip*"
                   "git-wip" "save" (concat "WIP from emacs: "
                                            (file-name-nondirectory
                                            buffer-file-name))
                   "--editor" "--"
                   (file-name-nondirectory buffer-file-name))
    (message (concat "Wrote and git-wip'd " (buffer-file-name)))))

(define-minor-mode git-wip-mode
  "Toggle git-wip mode.
With no argument, this command toggles the mode.
Non-null prefix argument turns on the mode.
Null prefix argument turns off the mode.

When git-wip mode is enabled, git-wip will be called every time
you save a buffer."
  ;; The initial value.
  nil
  ;; The indicator for the mode line.
  " WIP"
  :group 'git-wip

  ;; (de-)register our hook
  (if git-wip-mode
      (add-hook 'after-save-hook 'git-wip-after-save nil t)
    (remove-hook 'after-save-hook 'git-wip-after-save t)))

(defun git-wip-mode-if-git ()
  (interactive)
  (when (string= (vc-backend (buffer-file-name)) "Git")
    (git-wip-mode t)))

(add-hook 'find-file-hook 'git-wip-mode-if-git)

(provide 'git-wip-mode)
