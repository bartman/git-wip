-- this is a Neovim plugin that launches git-wip save on every buffer write
local vim = vim
local M = {}

-- Default configuration (matches exactly what you asked for)
M.defaults = {
  gpg_sign = false,
  untracked = true,
  ignored = true,
  filetypes = { "*" },
}

---@type table
M.config = M.defaults

---Setup function (automatically called by Lazy.nvim)
---@param opts? table
function M.setup(opts)
  -- Merge user config
  M.config = vim.tbl_deep_extend("force", M.defaults, opts or {})

  -- Register the BufWritePost autocmd once
  vim.api.nvim_create_autocmd("BufWritePost", {
    group = vim.api.nvim_create_augroup("GitWip", { clear = true }),
    pattern = "*",
    callback = M.GitWipBufWritePost,
    desc = "git-wip: run after buffer write",
  })
end

function M.GitWipBufWritePost()
  local fullpath = vim.api.nvim_buf_get_name(0)
  if fullpath == "" then
    return
  end

  -- Respect config.filetypes
  local ft = vim.bo.filetype
  local enabled = vim.tbl_contains(M.config.filetypes, "*")
    or vim.tbl_contains(M.config.filetypes, ft)
  if not enabled then
    return
  end

  local dir = vim.fn.fnamemodify(fullpath, ":h")      -- directory part
  local filename = vim.fn.fnamemodify(fullpath, ":t") -- just the filename

  -- Build the command using your config options
  local cmd = { "git-wip", "save", string.format("WIP from neovim for %s", filename) }

  -- Add config-driven flags (exactly as you wanted)
  if not M.config.gpg_sign then
    table.insert(cmd, "--no-gpg-sign")
  end
  if M.config.untracked then
    table.insert(cmd, "--untracked")
  end
  if M.config.ignored then
    table.insert(cmd, "--ignored")
  end

  table.insert(cmd, "--editor")
  table.insert(cmd, "--")
  table.insert(cmd, fullpath)   -- the actual file (full path)

  -- Run it from the correct directory + time it
  local start = vim.loop.hrtime()

  vim.system(cmd, { cwd = dir, text = true }, function(result)
    local elapsed = (vim.loop.hrtime() - start) / 1e9

    -- Notify on the main thread (never blocks Neovim)
    vim.schedule(function()
      if result.code == 0 then
        vim.notify(
          string.format("[git-wip] saved %s in %.3f sec", filename, elapsed),
          vim.log.levels.INFO
        )
      else
        vim.notify(
          string.format("[git-wip] failed for %s (exit %d)", filename, result.code),
          vim.log.levels.WARN
        )
      end
    end)
  end)
end

return M
