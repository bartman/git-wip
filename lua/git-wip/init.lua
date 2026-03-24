-- this is a Neovim plugin that launches git-wip save on every buffer write
local vim = vim
local M = {}

-- Configuration
M.defaults = {
  git_wip_path = "git-wip",  -- path to git-wip binary (can be absolute)
  gpg_sign     = nil,        -- true for --gpg-sign, false for --no-gpg-sign
  untracked    = nil,        -- true for --untracked, false for --no-untracked
  ignored      = nil,        -- true for --ignored, false for --no-ignored
  filetypes    = { "*" },
}

---@type table
M.config = M.defaults

---Helper for tri-state flags
local function add_tri_flag(cmd, value, positive, negative)
  if value == true then
    table.insert(cmd, positive)
  elseif value == false then
    table.insert(cmd, negative)
  end
  -- nil = do nothing (git-wip default)
end

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

  -- Build the command using config options
  local cmd = { M.config.git_wip_path, "save", string.format("WIP from neovim for %s", filename) }

  -- Tri-state flags
  add_tri_flag(cmd, M.config.gpg_sign, "--gpg-sign", "--no-gpg-sign")
  add_tri_flag(cmd, M.config.untracked, "--untracked", "--no-untracked")
  add_tri_flag(cmd, M.config.ignored, "--ignored", "--no-ignored")

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
