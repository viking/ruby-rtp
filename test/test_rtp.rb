require File.dirname(__FILE__) + '/test_helper'

class TestRfp < MiniTest::Unit::TestCase
  class TestSession < MiniTest::Unit::TestCase
    def setup
      @valid_options = {
        mode: RTP::Session::RTP_SESSION_SENDONLY,
        remote: "127.0.0.1:31337",
        local:  "127.0.0.1:8000"
      }
    end

    def create_session(options = {})
      RTP::Session.new(@valid_options.merge(options))
    end

    def test_constants
      assert_equal(0, RTP::Session::RTP_SESSION_RECVONLY)
      assert_equal(1, RTP::Session::RTP_SESSION_SENDONLY)
      assert_equal(2, RTP::Session::RTP_SESSION_SENDRECV)
    end

    def test_default_new
      create_session
    end

    def test_mode_option
      assert_raises(TypeError) { create_session(mode: 4)  }
      assert_raises(TypeError) { create_session(mode: -1) }
      assert_raises(TypeError) { create_session(mode: "pants") }
      create_session(mode: RTP::Session::RTP_SESSION_SENDRECV)
      create_session(mode: nil)
    end

    def test_remote_addr_option
      assert_raises(RuntimeError, "remote is required") { create_session(remote: nil) }
      assert_raises(TypeError, "remote must be a string") { create_session(remote: 123.456) }
      assert_raises(RuntimeError, "remote must have a valid port") { create_session(remote: "127.0.0.1") }
      assert_raises(RuntimeError, "remote must have a valid port") { create_session(remote: "127.0.0.1:") }
      assert_raises(RuntimeError, "remote must have a valid port") { create_session(remote: "127.0.0.1:-123") }
      assert_raises(RuntimeError, "remote must have a valid port") { create_session(remote: "127.0.0.1:65536") }
      assert_raises(RuntimeError, "remote must have a valid IP") { create_session(remote: "127.0.1:31337") }
      assert_raises(RuntimeError, "remote must have a valid IP") { create_session(remote: ":31337") }
      create_session(remote: "127.0.0.1:31337")
    end

    def test_local_addr_option
      create_session(local: nil)
      create_session(local: "127.0.0.1")
      assert_raises(RuntimeError, "local is required") { create_session(local: nil, mode: 0) }
      assert_raises(RuntimeError, "local is required") { create_session(local: nil, mode: 2) }
      assert_raises(TypeError, "local must be a string") { create_session(local: 123.456) }
      assert_raises(RuntimeError, "local must have a valid port") { create_session(local: "127.0.0.1:") }
      assert_raises(RuntimeError, "local must have a valid port") { create_session(local: "127.0.0.1:-123") }
      assert_raises(RuntimeError, "local must have a valid port") { create_session(local: "127.0.0.1:65536") }
      assert_raises(RuntimeError, "local must have a valid IP") { create_session(local: "127.0.1:31337") }
      assert_raises(RuntimeError, "local must have a valid IP") { create_session(local: ":31337") }
    end

    def test_remote_accessors
      s = create_session
      assert_equal("127.0.0.1", s.remote_addr)
      assert_equal(31337, s.remote_port)
      assert_equal("127.0.0.1", s.local_addr)
      assert_equal(8000, s.local_port)

      s = create_session(local: nil)
      assert_equal(nil, s.local_addr)
      assert_equal(nil, s.local_port)

      s = create_session(local: "127.0.0.1")
      assert_equal("127.0.0.1", s.local_addr)
      assert_equal(nil, s.local_port)
    end
  end
end
