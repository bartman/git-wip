-- this is a Neovim plugin that launches git-wip save on every buffer write
local vim = vim
local M = {}

-- Detect Neovim version for API compatibility
-- vim.system is available in Neovim 0.10+
local has_vim_system = vim.system ~= nil
local has_loop_hrtime = vim.loop and vim.loop.hrtime ~= nil

-- Configuration
M.defaults = {
  git_wip_path = "git-wip",  -- path to git-wip binary (can be absolute)
  gpg_sign     = nil,        -- true for --gpg-sign, false for --no-gpg-sign
  untracked    = nil,        -- true for --untracked, false for --no-untracked
  ignored      = nil,        -- true for --ignored, false for --no-ignored
  background   = nil,        -- true for async execution if supported, false for sync
  filetypes    = { "*" },
}

---@type table
M.config = M.defaults

---Wrapper for vim.loop.hrtime(), returns 0 if not available
local function get_hrtime()
  if has_loop_hrtime then
    return vim.loop.hrtime()
  else
    return 0
  end
end

---Helper for tri-state flags
local function add_tri_flag(cmd, value, positive, negative)
  if value == true then
    table.insert(cmd, positive)
  elseif value == false then
    table.insert(cmd, negative)
  end
  -- nil = do nothing (git-wip default)
end

---Helper: Build command array
local function build_command(display_name, filename)
  local cmd = { M.config.git_wip_path, "save", string.format("WIP from neovim for %s", display_name) }
  add_tri_flag(cmd, M.config.gpg_sign, "--gpg-sign", "--no-gpg-sign")
  add_tri_flag(cmd, M.config.untracked, "--untracked", "--no-untracked")
  add_tri_flag(cmd, M.config.ignored, "--ignored", "--no-ignored")
  table.insert(cmd, "--editor")
  if filename ~= nil then
    table.insert(cmd, "--")
    table.insert(cmd, filename)
  end
  return cmd
end

---Helper: Notify result
local function notify_result(display_name, code, elapsed)
  if code == 0 then
    local msg = "[git-wip] saved " .. display_name
    if elapsed and elapsed > 0 then
      msg = msg .. string.format(" in %.3f sec", elapsed)
    end
    vim.notify(msg, vim.log.levels.INFO)
  else
    local msg = "[git-wip] failed for " .. display_name .. " (exit " .. code .. ")"
    if M.config.background and not has_vim_system then
      msg = msg .. " (async not supported, ran sync)"
    end
    vim.notify(msg, vim.log.levels.WARN)
  end
end

---Helper: Run sync
local function run_sync(cmd, dir, display_name)
  local start = get_hrtime()
  local code = 0
  if has_vim_system then
    local job = vim.system(cmd, { cwd = dir, text = true })
    local result = job:wait()
    code = result.code
  else
    local shell_cmd = "cd " .. vim.fn.shellescape(dir) .. " && " .. cmd[1]
    for i = 2, #cmd do
      shell_cmd = shell_cmd .. " " .. vim.fn.shellescape(cmd[i])
    end
    vim.fn.system(shell_cmd)
    code = vim.v.shell_error
  end
  local elapsed = (get_hrtime() - start) / 1e9
  notify_result(display_name, code, elapsed)
end

---Helper: Run async
local function run_async(cmd, dir, display_name)
  local start = get_hrtime()
  vim.system(cmd, {
    cwd = dir,
    text = true,
    on_exit = function(result)
      local elapsed = (get_hrtime() - start) / 1e9
      vim.schedule(function()
        notify_result(display_name, result.code, elapsed)
      end)
    end
  })
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

  -- Register commands
  vim.api.nvim_create_user_command("Wip", function()
    local fullpath = vim.api.nvim_buf_get_name(0)
    if fullpath == "" then
      vim.notify("[git-wip] no file name for current buffer", vim.log.levels.ERROR)
      return
    end
    local dir = vim.fn.fnamemodify(fullpath, ":h")
    local filename = vim.fn.fnamemodify(fullpath, ":t")
    M.RunGitWip(dir, filename)
  end, {
    desc = "Save WIP snapshot for the current buffer",
  })

  vim.api.nvim_create_user_command("WipAll", function()
    local dir = vim.fn.getcwd()
    M.RunGitWip(dir, nil)
  end, {
    desc = "Save WIP snapshot for all changes in the current directory",
  })
end

function M.RunGitWip(dir, filename)
  local display_name = filename or '*'
  local cmd = build_command(display_name, filename)
  if M.config.background and has_vim_system then
    run_async(cmd, dir, display_name)
  else
    run_sync(cmd, dir, display_name)
  end
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

  M.RunGitWip(dir, filename)
end

return M
