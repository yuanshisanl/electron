const { autoUpdater } = require('electron').remote
const { ipcRenderer } = require('electron')
const { expect } = require('chai')

describe('autoUpdater module', function () {
  // XXX(alexeykuzmin): Calling `.skip()` in a 'before' hook
  // doesn't affect nested 'describe's
  beforeEach(function () {
    // Skip autoUpdater tests in MAS build.
    if (process.mas) {
      this.skip()
    }
  })

  describe('checkForUpdates', function () {
    it('emits an error on Windows when called the feed URL is not set', function (done) {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      ipcRenderer.once('auto-updater-error', (event, message) => {
        expect(message).to.equal('Update URL is not set')
        done()
      })
      autoUpdater.initialize('')
      autoUpdater.checkForUpdates()
    })
  })

  describe('feedURL', () => {
    it('returns a falsey value by default', () => {
      // TODO(jkleinsc): Remove in 7.0
      expect(autoUpdater.getFeedURL()).to.equal('')

      expect(autoUpdater.feedURL).to.equal('')
    })

    it('correctly fetches the previously set FeedURL via initialize', function (done) {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      const updateURL = 'https://fake-update.electron.io'
      autoUpdater.initialize(updateURL)

      // TODO(jkleinsc): Remove in 7.0
      expect(autoUpdater.getFeedURL()).to.equal(updateURL)

      expect(autoUpdater.feedURL).to.equal(updateURL)
      done()
    })
  })

  it('correctly fetches the previously set FeedURL via feedURL property', function (done) {
    if (process.platform !== 'win32') {
      // FIXME(alexeykuzmin): Skip the test.
      // this.skip()
      return done()
    }

    const updateURL = 'https://fake-update.electron.io'
    autoUpdater.feedURL = updateURL

    // TODO(jkleinsc): Remove in 7.0
    expect(autoUpdater.getFeedURL()).to.equal(updateURL)

    expect(autoUpdater.feedURL).to.equal(updateURL)
    done()
  })

  describe('initialize', function () {
    describe('on Mac or Windows', () => {
      const noThrow = (fn) => {
        try { fn() } catch (err) {}
      }

      before(function () {
        if (process.platform !== 'win32' && process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('sets url successfully using old (url, headers) syntax', () => {
        const url = 'http://electronjs.org'
        noThrow(() => autoUpdater.initialize(url, { header: 'val' }))

        // TODO(jkleinsc): Remove in 7.0
        expect(autoUpdater.getFeedURL()).to.equal(url)

        expect(autoUpdater.feedURL).to.equal(url)
      })

      it('throws if no url is provided when using the old style', () => {
        expect(() => autoUpdater.initialize(),
          err => err.message.includes('Expected an options object with a \'url\' property to be provided') // eslint-disable-line
        ).to.throw()
      })

      it('sets url successfully using new ({ url }) syntax', () => {
        const url = 'http://mymagicurl.local'
        noThrow(() => autoUpdater.initialize({ url }))

        // TODO(jkleinsc): Remove in 7.0
        expect(autoUpdater.getFeedURL()).to.equal(url)

        expect(autoUpdater.feedURL).to.equal(url)
      })

      it('throws if no url is provided when using the new style', () => {
        expect(() => autoUpdater.initialize({ noUrl: 'lol' }),
          err => err.message.includes('Expected options object to contain a \'url\' string property in initialize call') // eslint-disable-line
        ).to.throw()
      })
    })

    describe('on Mac', function () {
      const isServerTypeError = (err) => err.message.includes('Expected serverType to be \'default\' or \'json\'')

      before(function () {
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('emits an error when the application is unsigned', done => {
        ipcRenderer.once('auto-updater-error', (event, message) => {
          expect(message).equal('Could not get code signature for running application')
          done()
        })
        autoUpdater.initialize('')
      })

      it('does not throw if default is the serverType', () => {
        expect(() => autoUpdater.initialize({ url: '', serverType: 'default' }),
          isServerTypeError
        ).to.not.throw()
      })

      it('does not throw if json is the serverType', () => {
        expect(() => autoUpdater.initialize({ url: '', serverType: 'default' }),
          isServerTypeError
        ).to.not.throw()
      })

      it('does throw if an unknown string is the serverType', () => {
        expect(() => autoUpdater.initialize({ url: '', serverType: 'weow' }),
          isServerTypeError
        ).to.throw()
      })
    })
  })

  describe('quitAndInstall', () => {
    it('emits an error on Windows when no update is available', function (done) {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      ipcRenderer.once('auto-updater-error', (event, message) => {
        expect(message).to.equal('No update available, can\'t quit and install')
        done()
      })
      autoUpdater.quitAndInstall()
    })
  })

  describe('error event', () => {
    it('serializes correctly over the remote module', function (done) {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      autoUpdater.once('error', error => {
        expect(error).to.be.an.instanceof(Error)
        expect(Object.getOwnPropertyNames(error)).to.deep.equal(['stack', 'message', 'name'])
        done()
      })

      autoUpdater.initialize('')

      if (process.platform === 'win32') {
        autoUpdater.checkForUpdates()
      }
    })
  })
})
