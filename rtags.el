(defun rtags-setup-hooks () (interactive)
  (remove-hook 'after-save-hook 'rtags-sync-all-open-files)
  (remove-hook 'find-file-hooks 'rtags-sync-all-open-files)
  (add-hook 'after-save-hook 'rtags-sync-all-open-files)
  (add-hook 'find-file-hooks 'rtags-sync-all-open-files)
  )

(defgroup gtags nil
  "Minor mode for rtags."
  :group 'tools
  :prefix "gtags-")

(defcustom rtags-autostart nil
  "Whether or not to autostart rtags from emacs"
  :type 'boolean
  :group 'rtags)

(defcustom rtags-enable t
  "Whether or rtags is enabled"
  :type 'boolean
  :group 'rtags)

(defun rtags-autostart-arg()
  (if rtags-autostart
      "--autostart"
    ""))

(defun rtags-load-file () (interactive)
  (start-process "rtags-load" nil "rtags" "--timeout=1000" (rtags-autostart-arg) "--command=load" (buffer-file-name)))

(defun rtags-conditional-load-file (&optional buffer) (interactive)
  (if (or (eq major-mode 'c++-mode) (eq major-mode 'c-mode))
      (rtags-load-file))
  )

(defun rtags-sync-all-open-files() (interactive)
  ;; (if (and rtags-enable (executable-find "rtags"))
  ;;     (let (paths)
  ;;       (dolist (buffer (buffer-list))
  ;;         (with-current-buffer buffer
  ;;           (if (and
  ;;                (or (eq major-mode 'c++-mode) (eq major-mode 'c-mode))
  ;;                (not (string-match "\\.\\(hxx\\|hpp\\|tcc\\|h\\)?$" (buffer-file-name))))
  ;;               (add-to-list 'paths (buffer-file-name)))))
  ;;       (if paths
  ;;           (apply 'start-process "rtags-load" nil "rtags" "--timeout=1000" (rtags-autostart-arg) "--command=load" paths))
  ;;       )
  ;;   )
  nil
  )

(defun rtags-goto-symbol-at-point()
  (interactive)
  (let ((bufname (buffer-file-name))
        (line (int-to-string (line-number-at-pos)))
        (column nil))
    (save-excursion
      (if (looking-at "[0-9A-Za-z_]")
          (progn
            ;; (message (concat (int-to-string (point)) " 1 " (int-to-string (- (point) (point-at-bol) -1))))
            (while (and (> (point) 0) (looking-at "[0-9A-Za-z_]"))
              (backward-char))
            ;; (message (concat (int-to-string (point)) " 2 " (int-to-string (- (point) (point-at-bol) -1))))
            (if (not (looking-at "[0-9A-Za-z_]"))
                (forward-char))
            ;; (message (concat (int-to-string (point)) " 3 " (int-to-string (- (point) (point-at-bol) -1))))
            (setq column (int-to-string (- (point) (point-at-bol) -1))))))

            ;; /foo/bar:12:13
    (with-temp-buffer
      (call-process (executable-find "rtags") nil t nil "--timeout=50" "--command=followsymbol"
                    bufname (concat "--line=" line) (concat "--column=" column))
      (message (buffer-string))
      (string-match "\\(.*\\):\\([0-9]+\\):\\([0-9]+\\)" (buffer-string))
      (if (match-beginning 1)
          (progn
            (setq line (string-to-int (match-string 2 (buffer-string))))
            (setq column (string-to-int (match-string 3 (buffer-string))))
            (find-file (match-string 1 (buffer-string)))
        ;; (message (concat (int-to-string line) " " (int-to-string column)))
            (goto-char (point-min))
            (forward-line (- line 1))
            (forward-char (- column 1)))
        (message "Can't follow symbol"))
      )
    )
  )

(provide 'rtags)