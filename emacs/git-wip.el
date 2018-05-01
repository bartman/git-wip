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
