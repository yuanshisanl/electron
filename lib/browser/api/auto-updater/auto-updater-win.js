'use strict'

const { app, deprecate } = require('electron')
const { EventEmitter } = require('events')
const squirrelUpdate = require('@electron/internal/browser/api/auto-updater/squirrel-update-win')

class AutoUpdater extends EventEmitter {
  quitAndInstall () {
    if (!this.updateAvailable) {
      return this.emitError('No update available, can\'t quit and install')
    }
    squirrelUpdate.processStart()
    app.quit()
  }

  initialize (options) {
    if (typeof options === 'object') {
      if (typeof options.url === 'string') {
        this.options = options
      } else {
        throw new Error('Expected options object to contain a \'url\' string property in initialize call')
      }
    } else if (typeof options === 'string') {
      this.options = {
        updateURL: options
      }
    } else {
      throw new Error('Expected an options object with a \'url\' property to be provided')
    }
    this.feedUrl = this.options.updateURL
  }

  checkForUpdates () {
    if (!this.options.updateURL) {
      return this.emitError('Update URL is not set')
    }
    if (!squirrelUpdate.supported()) {
      return this.emitError('Can not find Squirrel')
    }
    this.emit('checking-for-update')
    squirrelUpdate.checkForUpdate(this.options.updateURL, (error, update) => {
      if (error != null) {
        return this.emitError(error)
      }
      if (update == null) {
        return this.emit('update-not-available')
      }
      this.updateAvailable = true
      this.emit('update-available')
      squirrelUpdate.update(this.options.updateURL, (error) => {
        if (error != null) {
          return this.emitError(error)
        }
        const { releaseNotes, version } = update
        // Date is not available on Windows, so fake it.
        const date = new Date()
        this.emit('update-downloaded', {}, releaseNotes, version, date, this.options.updateURL, () => {
          this.quitAndInstall()
        })
      })
    })
  }

  // Private: Emit both error object and message, this is to keep compatibility
  // with Old APIs.
  emitError (message) {
    this.emit('error', new Error(message), message)
  }
}

deprecate.function(AutoUpdater, 'setFeedURL', 'initialize')
deprecate.fnToProperty(AutoUpdater, 'feedURL', '_getFeedURL')

module.exports = new AutoUpdater()
