require File.dirname(__FILE__) + '/test_helper'

class TestRfp < MiniTest::Unit::TestCase
  class TestSession < MiniTest::Unit::TestCase
    def setup
      @send_options = {
        mode: RTP::Session::RTP_SESSION_SENDONLY,
        remote: "127.0.0.1:31337", local: nil, block: false,
        connected: true
      }
      @recv_options = {
        mode: RTP::Session::RTP_SESSION_RECVONLY,
        local: "127.0.0.1:31337", remote: nil, block: true,
        connected: true
      }
    end

    def create_send_session(options = {})
      s = RTP::Session.new(@send_options.merge(options))
      yield s   if block_given?
      s.close
    end

    def create_recv_session(options = {})
      s = RTP::Session.new(@recv_options.merge(options))
      yield s   if block_given?
      s.close
    end

    def test_constants
      assert_equal(0, RTP::Session::RTP_SESSION_RECVONLY)
      assert_equal(1, RTP::Session::RTP_SESSION_SENDONLY)
      assert_equal(2, RTP::Session::RTP_SESSION_SENDRECV)
    end

    def test_send_session
      create_send_session
    end

    def test_recv_session
      create_recv_session
    end

    def test_mode_option
      assert_raises(TypeError) { create_send_session(mode: 4)  }
      assert_raises(TypeError) { create_send_session(mode: -1) }
      assert_raises(TypeError) { create_send_session(mode: "pants") }
      create_send_session(mode: RTP::Session::RTP_SESSION_SENDONLY)
      create_send_session(mode: nil)
    end

    def test_remote_addr_option
      assert_raises(RuntimeError, "remote address and port are required") { create_send_session(remote: nil) }
      assert_raises(TypeError, "remote must be a string") { create_send_session(remote: 123.456) }
      assert_raises(RuntimeError, "remote port is required") { create_send_session(remote: "127.0.0.1") }
      assert_raises(RuntimeError, "remote port is invalid") { create_send_session(remote: "127.0.0.1:") }
      assert_raises(RuntimeError, "remote port is invalid") { create_send_session(remote: "127.0.0.1:-123") }
      assert_raises(RuntimeError, "remote port is invalid") { create_send_session(remote: "127.0.0.1:65536") }
      assert_raises(RuntimeError, "remote must have a valid IP") { create_send_session(remote: "127.0.0.137") }
      assert_raises(RuntimeError, "remote address and port are required") { create_send_session(remote: ":31337") }
    end

    def test_local_addr_option
      assert_raises(RuntimeError, "local address and port are required") { create_recv_session(local: nil) }
      assert_raises(TypeError, "local must be a string") { create_recv_session(local: 123.456) }
      assert_raises(RuntimeError, "local port is required") { create_recv_session(local: "127.0.0.1") }
      assert_raises(RuntimeError, "local port is invalid") { create_recv_session(local: "127.0.0.1:") }
      assert_raises(RuntimeError, "local port is invalid") { create_recv_session(local: "127.0.0.1:-123") }
      assert_raises(RuntimeError, "local port is invalid") { create_recv_session(local: "127.0.0.1:65536") }
      assert_raises(RuntimeError, "local must have a valid IP") { create_recv_session(local: "127.0.1:31337") }
      assert_raises(RuntimeError, "local address and port are required") { create_recv_session(local: ":31337") }
    end

    def test_block_option
      create_send_session(block: false)
      create_recv_session(block: "huge")
    end

    def test_connected_option
      create_send_session(connected: false)
      create_recv_session(connected: "huge")
    end

    def test_closed
      create_send_session do |s|
        s.close
        assert(s.closed?)
      end
    end

    def test_accessors_on_sendonly
      create_send_session do |s|
        assert_equal("127.0.0.1", s.remote_addr)
        assert_equal(31337, s.remote_port)
        assert_equal(nil, s.local_addr)
        assert_equal(nil, s.local_port)
      end
    end

    def test_accessors_on_recvonly
      create_recv_session do |s|
        assert_equal("127.0.0.1", s.local_addr)
        assert_equal(31337, s.local_port)
      end
    end

    def test_sending_and_receiving
      create_send_session do |sending|
        sending.send_file(File.dirname(__FILE__) + '/fixtures/KDE-Sys-Warning.ogg')
      end
    end
  end
end
