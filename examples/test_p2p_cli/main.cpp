#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include <mqas/tools/stream/p2p_lobby_client.h>
#include <curses.h>
#include <stack>
#include <mqas/io/idle.h>
#include <mqas/tools/stream/p2p_helper.h>
using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP


class LobbyStream : public tools::p2p::P2PLobbyClientStream {
protected:
};

using StreamType = core::StreamVariant<
	core::StreamVariantPair<1, LobbyStream>,
	core::StreamVariantPair<2, mqas::tools::p2p::P2PHelperStream>>;

enum class ui_state {
	none = 0,
	main = 1,
	select_peer = 2,
	pop_want_connect = 3,
	select_peer_not_find = 4,
	get_respond = 5,
};

struct tui
{
	void init();
	void init_stream(std::shared_ptr<LobbyStream> stream);
	void draw();
	void handle_input();
	void destroy();
	bool has_peer(uint32_t id);

	void on_get_peer_list(std::shared_ptr<mqas::tools::proto::p2p::RespondPeerList> list);
	void on_peer_want_connect(const mqas::tools::proto::p2p::PeerData&);
	void on_get_respond(const std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer>& code);
	void append_state(ui_state state);
	ui_state pop_state();
	ui_state current_state() const;
protected:
	std::stack<ui_state> stack;
	std::shared_ptr<LobbyStream> stream;
	std::shared_ptr<mqas::tools::proto::p2p::RespondPeerList> peer_list;
	WINDOW* win;
	uint32_t input_peer_id;
	std::vector<mqas::tools::proto::p2p::PeerData> want_connect_list;
	std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer> respond_code;
};

int main(int argc, const char** argv)
{
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	tui ui;
	ui.init();
	core::engine_base<core::engine<core::Connect<StreamType>>> e(io_cxt);
	try {
		e.init("conf.txt", core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
		sockaddr addr{};
		io::Ip::str2addr_ipv4("127.0.0.1", 8084, addr);
		auto c = e.get_engine()->connect(addr, N_LSQVER);
		auto conn = c.lock();
		auto t = io_cxt.make_handle<io::Timer>();
		std::weak_ptr<StreamType> stream_out;
		conn->make_stream([&io_cxt, &stream_out,&ui](std::weak_ptr<StreamType> stream) {
			stream_out = stream;
			auto s = stream.lock();
			mqas::tools::proto::p2p::ReqRegistePeer msg;
			s->req_change<LobbyStream, mqas::tools::p2p::ReqRegistePeerPair>(msg);
			auto lobby_stream = s->get_holds_stream<LobbyStream>();
			lobby_stream->on_change_helper = [stream](const mqas::tools::proto::p2p::ReqRespondPeerReqConnect& msg)
			{
				auto s = stream.lock();
				s->req_change<mqas::tools::p2p::P2PHelperStream, mqas::tools::p2p::ReqRespondPeerReqConnectPair>(msg);
			};
			lobby_stream->on_change_helper_by_req = [stream](const mqas::tools::proto::p2p::ReqConnectPeer& msg)
			{
				auto s = stream.lock();
				s->req_change<mqas::tools::p2p::P2PHelperStream, mqas::tools::p2p::ReqConnectPeerPair>(msg);
			};
			ui.init_stream(lobby_stream);
		});
		t->start([&stream_out](mqas::io::Timer* t) {
			auto s = stream_out.lock();
			if (s && s->has_holds_stream())
			{
				s->get_holds_stream<LobbyStream>()->req_peer_list();
			}
		}, 1000, -1);
		auto idle_for_ui = io_cxt.make_handle<io::Idle>();
		idle_for_ui->start([&ui](io::Idle* idle){
			ui.draw();
			ui.handle_input();
		});
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	ui.destroy();
	return 0;
}

void tui::init()
{
	initscr();

	// Disable line buffering and echo (we'll handle input manually)
	cbreak();
	noecho();
	nodelay(stdscr,TRUE);
	refresh();
	append_state(ui_state::main);
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	win = newwin(rows, cols, 0, 0);
	input_peer_id = 0;
}

void tui::append_state(ui_state state)
{
	stack.push(state);
}
ui_state tui::pop_state()
{
	auto res = current_state();
	if (!stack.empty())
		stack.pop();
	return res;
}

ui_state tui::current_state() const
{
	return stack.empty() ? ui_state::none : stack.top();
}

void tui::init_stream(std::shared_ptr<LobbyStream> stream)
{
	this->stream = stream;
	stream->on_get_peer_list_signal.connect(sigc::mem_fun(*this,&tui::on_get_peer_list));
	stream->on_want_connect.connect(sigc::mem_fun(*this, &tui::on_peer_want_connect));
	stream->on_get_respond.connect(sigc::mem_fun(*this, &tui::on_get_respond));
}

void tui::on_get_peer_list(std::shared_ptr<mqas::tools::proto::p2p::RespondPeerList> list)
{
	append_state(ui_state::select_peer);
	peer_list = list;
}

void tui::on_peer_want_connect(const mqas::tools::proto::p2p::PeerData& peer)
{
	want_connect_list.emplace_back(peer);
	append_state(ui_state::pop_want_connect);
}

void tui::on_get_respond(const std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer>& code)
{
	respond_code = code;
	append_state(ui_state::get_respond);
}

void tui::draw()
{
	werase(win);
	box(win, 0, 0);
	wmove(win, 1, 1);
	switch (current_state())
	{
	case ui_state::main:
		wprintw(win, "Input R to request peer list!");
		break;
	case ui_state::select_peer:
	{
		auto y = 2;
		wprintw(win,"peer list:");
		if (peer_list == nullptr)
			return;
		for (int i = 0; i < peer_list->peer_list_size(); ++i)
		{
			auto it = peer_list->peer_list().Get(i);
			wmove(win, y++, 1);
			wprintw(win, "%u ----- %s",it.id(),it.name().c_str());
		}
		wmove(win, y++, 1);
		wprintw(win, "Input peer id to request connect! current = %u,input Enter end",input_peer_id);
	}
		break;
	case ui_state::select_peer_not_find:
		wprintw(win, "Not found peer id!!!");
		break;
	case ui_state::pop_want_connect:
		if (!want_connect_list.empty())
		{
			const auto& peer = want_connect_list[0];
			wprintw(win, "%u - %s want connect.", peer.id(),peer.name().c_str());
			wmove(win, 2, 1);
			wprintw(win,"Input y/n to respond");
		}
		break;
	case ui_state::get_respond:
		wprintw(win, "Get respond %s from %d",mqas::tools::proto::p2p::RetCode_Name(respond_code->ret()).c_str(),respond_code->peer_id());
		break;
	default:
		break;
	}
	wrefresh(win);
}

void tui::handle_input()
{
	auto c = getch();
	switch (current_state())
	{
	case ui_state::main:
		if (c == 'r')
			stream->req_peer_list();
		break;
	case ui_state::select_peer:
	{
		if (c >= '0' && c <= '9')
			input_peer_id = (input_peer_id * 10) + (c - '0');
		if (c == 8) //backspace
			input_peer_id = input_peer_id / 10;
		if (c == 10) //enter
		{
			if (has_peer(input_peer_id))
				stream->req_connect(input_peer_id);
			else
				append_state(ui_state::select_peer_not_find);
			input_peer_id = 0;
		}
	}
		break;
	case ui_state::select_peer_not_find:
		break;
	case ui_state::pop_want_connect:
		if (!want_connect_list.empty())
		{
			if (c == 'y' || c == 'n')
			{
				const auto& peer = want_connect_list[0];
				auto agree = c == 'y';
				stream->req_respond(peer.id(), agree);
				want_connect_list.erase(want_connect_list.begin());
			}
		}
		else
			pop_state();
		return;
		break;
	case ui_state::get_respond:
		break;
	default:
		break;
	}
	if (c == 27)
	{
		pop_state();
		if (stack.empty())
			IsRunning() = false;
	}
}

bool tui::has_peer(uint32_t id)
{
	if (peer_list == nullptr)
		return false;
	for (int i = 0; i < peer_list->peer_list_size(); ++i)
	{
		auto it = peer_list->peer_list().Get(i);
		if (it.id() == id)
			return true;
	}
	return false;
}

void tui::destroy()
{
	delwin(win);
	endwin();
}
