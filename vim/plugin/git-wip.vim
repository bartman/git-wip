" ---------------------------------------------------------------------------
" git wip stuff

if !exists('g:git_wip_verbose')
        let g:git_wip_verbose = 0
endif

function! GitWipSave()
        let dir = expand("%:p:h")
        let file = expand("%:t")
        let out = system('cd ' . dir . ' && git wip save "WIP from vim" --editor -- "' . file . '" 2>&1')
        let err = v:shell_error
        if err
                echohl Error
                echo out
                echohl None
        elseif g:git_wip_verbose
                echo out
        endif
endf

silent! !git wip -h >/dev/null 2>&1
if !v:shell_error
        augroup git-wip
                autocmd!
                autocmd BufWritePost * :call GitWipSave()
        augroup END
endif

