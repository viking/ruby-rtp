require File.dirname(__FILE__) + '/test_helper'

class TestRfp < MiniTest::Unit::TestCase
  class TestSession < MiniTest::Unit::TestCase
    def setup
      @valid_options = {
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
    end
  end
end
