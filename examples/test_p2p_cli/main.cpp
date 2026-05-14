#include <mqas/context.h>
#include <mqas/io/context.h>
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include <mqas/tools/stream/p2p_lobby_client.h>
#include <curses.h>
#include <stack>
#include <mqas/io/idle.h>
#include <mqas/tools/stream/p2p_helper_client.h>
#include <mqas/comm/engine.h>
#include "p2p_chat.h"
#include <mqas/comm/locator.h>
#include <algorithm>
#include <mqas/core/sub_engine.h>
#include <mqas/tools/stream/relay_stream_client.h>

using namespace mqas;


#define KEY_ESC 27
#define KEY_Backspace 8
#define KEY_Enter 10

const std::string none_str = "none";
const int zero = 0;

class LobbyStream : public tools::p2p::P2PLobbyClientStream {
protected:
};

using StreamType = core::StreamVariant<
	core::StreamVariantPair<1, LobbyStream>,
	core::StreamVariantPair<2, mqas::tools::p2p::P2PHelperClientStream>>;

using P2PStreamType = core::StreamVariant<
	core::StreamVariantPair<1,P2PChatStream>>;

using RelayStreamType = core::StreamVariant<
	core::StreamVariantPair<1,mqas::tools::RelayStreamClient>>;

using P2PEngineType = core::sub_engine<core::engine<core::Connect<P2PStreamType>>>;
using P2PEngineRelayType = core::sub_engine<core::engine<core::Connect<P2PStreamType>>,core::engine_driver,tools::RelayStreamClient>;

using RelayEngineType = core::sub_engine<core::engine<core::Connect<RelayStreamType>>>;

enum class ui_state {
	none = 0,
	main = 1,
	select_peer = 2,
	pop_want_connect = 3,
	select_peer_not_find = 4,
	get_respond = 5,
	helper_main = 6,
	helper_result_failed = 7,
	p2p_main = 8,
	p2p_chat = 9,
};

struct p2p_chat_cxt {
	std::string peer_name;
	std::vector<std::pair<uint8_t,std::string>> msg_list;
	uint16_t height = 0;
	uint16_t width = 0;
	uint16_t input_pos = 0;
	uint16_t min = 0;
	uint16_t max = 0;
	std::string input_text;

	void scroll_end()
	{
		size_t total_messages = msg_list.size();
		if (height >= total_messages) {
			min = 0;
			max = total_messages <= 0 ? 0 : total_messages - 1;
		}
		else {
			max = total_messages - 1;
			min = max - height;
		}
	}

	void scroll_up() {
		if (min > 0) {
			min--;
			max--;
		}
	}

	void scroll_down() {
		if (max < msg_list.size() - 1) {
			min++;
			max++;
		}
	}
};

struct tui
{
	void init();
	void init_stream(std::shared_ptr<LobbyStream> stream);
	void reg_helper_stream(std::shared_ptr<mqas::tools::p2p::P2PHelperClientStream> stream);
	void draw();
	void handle_input();
	void destroy();
	bool has_peer(uint32_t id);
	std::optional<mqas::tools::proto::p2p::PeerData> get_peer(uint32_t id) const;
protected:
	void on_change_helper_result(const std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer>& respond);
	void on_helper_connect_to(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectPeerData>& msg);
	void on_get_peer_list(std::shared_ptr<mqas::tools::proto::p2p::RespondPeerList> list);
	void on_peer_want_connect(const mqas::tools::proto::p2p::PeerData&);
	void on_get_respond(const std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer>& code);
	void on_helper_result(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>& msg, std::shared_ptr<io::UdpSocket>);
	void append_state(ui_state state);
	void quit_helper();
	void quit_p2p();
	void on_new_p2p_connect(std::shared_ptr<core::Connect<P2PStreamType>> conn,bool is_server);
	void on_new_p2p_stream(std::shared_ptr<P2PStreamType> stream, bool is_server);
	void on_recive_p2p_msg(const std::string&,const std::string&);
	void on_p2p_peer_quit(std::shared_ptr<core::IStreamVariant> stream);
	void on_p2p_connected(const std::string&);
	void clean_up_p2p(bool active = true);
	ui_state pop_state();
	ui_state current_state() const;
	template<typename SOCK>
	requires core::IsVaildSocket<SOCK>
	void launch_p2p(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>& msg,std::shared_ptr<SOCK> sock);
	bool launch_relay(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>& msg,std::function<void(bool)> callback,
		std::shared_ptr<io::UdpSocket> sock);
	void clean_relay();
public:
	//conf
	std::shared_ptr<toml::value> config;
protected:
	std::stack<ui_state> stack;
	std::shared_ptr<LobbyStream> stream;
	std::shared_ptr<mqas::tools::p2p::P2PHelperClientStream> helper_stream;
	std::shared_ptr<mqas::tools::proto::p2p::RespondPeerList> peer_list;
	WINDOW* win;
	uint32_t input_peer_id;
	std::vector<mqas::tools::proto::p2p::PeerData> want_connect_list;
	std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer> respond_code;
	std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer> respond_change_helper;
	std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult> helper_result;
	std::vector<std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectPeerData>> try_connect_list;
	std::shared_ptr<core::engine_base_interface> p2p_engine;
	std::weak_ptr<P2PChatStream> p2p_stream;
	std::shared_ptr<p2p_chat_cxt> p2p_cxt;
	int rows, cols;
	std::shared_ptr<io::Idle> idle;
	std::vector<std::function<void()>> lazy_task;
	std::shared_ptr<io::Timer> p2p_server_wait_timer;
	sockaddr p2p_addr;
	//relay
	std::shared_ptr<RelayEngineType> relay_engine;
	std::shared_ptr<mqas::tools::RelayStreamClient> relay_stream;
	bool use_relay = false;
};

int main(int argc, const char** argv)
{
	Context<core::InitFlags::BOTH> context;
	io::Context io_cxt;
	comm::locator::inst()->deposit<std::reference_wrapper<io::Context>>(io_cxt);
	tui ui;
	ui.init();
	core::sub_engine<core::engine<core::Connect<StreamType>>> e(io_cxt);
	try {
		e.init(argc > 1 ? argv[1] : "conf.txt", core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
		sockaddr addr{};
		auto ip = toml::find<std::string>(*e.get_engine()->get_config(),"client", "ip");
		auto port = toml::find<int>(*e.get_engine()->get_config(), "client", "port");
		if(!io::Ip::str2addr_ipv4(ip.c_str(), port, addr))
			throw new std::exception("Not found target address!");
		auto c = e.get_engine()->connect(addr, N_LSQVER);
		e.get_engine()->whitelist_addr.push_back(addr);
		e.get_engine()->whitelist_port.push_back(io::Ip::addr_get_port(addr));
		ui.config = e.get_engine()->get_config();
		auto conn = c.lock();
		auto t = io_cxt.make_handle<io::Timer>();
		std::weak_ptr<StreamType> stream_out;
		conn->make_stream([&e,&io_cxt, &stream_out,&ui](std::weak_ptr<StreamType> stream) {
			stream_out = stream;
			auto s = stream.lock();
			mqas::tools::proto::p2p::ReqRegistePeer msg;
			s->req_change<LobbyStream, mqas::tools::p2p::ReqRegistePeerPair>(msg);
			auto lobby_stream = s->get_holds_stream<LobbyStream>();
			lobby_stream->on_change_helper = [stream,&ui](const mqas::tools::proto::p2p::ReqRespondPeerReqConnect& msg)
			{
				auto s = stream.lock();
				s->req_change< mqas::tools::p2p::P2PHelperClientStream, mqas::tools::p2p::ReqRespondPeerReqConnectPair>(msg);
				ui.reg_helper_stream(s->get_holds_stream<mqas::tools::p2p::P2PHelperClientStream>());
			};
			lobby_stream->on_change_helper_by_req = [stream,&ui](const mqas::tools::proto::p2p::ReqConnectPeer& msg)
			{
				auto s = stream.lock();
				s->req_change< mqas::tools::p2p::P2PHelperClientStream, mqas::tools::p2p::ReqConnectPeerPair>(msg);
				ui.reg_helper_stream(s->get_holds_stream<mqas::tools::p2p::P2PHelperClientStream>());
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
	start_color();
	// Disable line buffering and echo (we'll handle input manually)
	cbreak();
	noecho();
	nodelay(stdscr,TRUE);
	refresh();
	append_state(ui_state::main);
	getmaxyx(stdscr, rows, cols);
	win = newwin(rows, cols, 0, 0);
	input_peer_id = 0;

	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);

	idle = comm::locator::inst()->get_ref<io::Context>().value().get().make_shared<io::Idle>();
	idle->start([this](io::Idle*) {
		for(auto& t : lazy_task)
			t();
		lazy_task.clear();
	});
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
	stream->on_peer_list_signal.connect(sigc::mem_fun(*this, &tui::on_get_peer_list));
	stream->on_request_connect_signal.connect(sigc::mem_fun(*this, &tui::on_peer_want_connect));
	stream->on_connect_response_signal.connect(sigc::mem_fun(*this, &tui::on_get_respond));
}

void tui::on_get_peer_list(std::shared_ptr<mqas::tools::proto::p2p::RespondPeerList> list)
{
	append_state(ui_state::select_peer);
	peer_list = list;
}

void tui::on_peer_want_connect(const mqas::tools::proto::p2p::PeerData& peer)
{
	auto curr = current_state();
	if(curr == ui_state::select_peer)
	{ 
		want_connect_list.emplace_back(peer);
		append_state(ui_state::pop_want_connect);
	}
	else {
		stream->req_respond(peer.id(), false);
	}
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
	int y = 1;
	wmove(win, y, 1);
	switch (current_state())
	{
	case ui_state::main:
		wprintw(win, "Input R to request peer list!");
		break;
	case ui_state::select_peer:
	{
		wprintw(win,"peer list:");
		if (peer_list == nullptr)
			return;
		for (int i = 0; i < peer_list->peer_list_size(); ++i)
		{
			auto it = peer_list->peer_list().Get(i);
			wmove(win, ++y, 1);
			wprintw(win, "%u ----- %s",it.id(),it.name().c_str());
		}
		wmove(win, ++y, 1);
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
	case ui_state::helper_main:
	{
		auto peer = get_peer(respond_change_helper->peer_id());
		std::string name = none_str;
		if (peer)
			name = peer.value().name();
		wprintw(win, "Helper Main peer = %d,name = %s", respond_change_helper->peer_id(),name.c_str());
		for (int i = 0; i < this->try_connect_list.size(); ++i)
		{
			wmove(win, ++y, 1);
			wprintw(win, "try connect to %s:%d", try_connect_list[i]->connect_data().ip().c_str(), try_connect_list[i]->connect_data().port());
		}
		if(helper_result && helper_result->use_relay())
		{
			wmove(win, ++y, 1);
			wprintw(win, "use relay connect %s:%d", helper_result->peer_addr().ip().c_str(), helper_result->peer_addr().port());
		}
	}
		break;
	case ui_state::helper_result_failed:
		wprintw(win, "Get p2p failed %s from %d",helper_result->reason().c_str(), helper_result->peer_id());
		break;
	case ui_state::p2p_main:
		wprintw(win, "Get success p2p ready to connect to %d addr is %s:%d",helper_result->peer_id(),
			helper_result->address().ip().c_str(), helper_result->address().port());
		wmove(win, ++y, 1);
		wprintw(win, helper_result->is_server() ? "wating connect" : "connecting...");
		//todo sync p2p connect state
		break;
	case ui_state::p2p_chat:
	{
		wmove(win,0,0);
		wprintw(win,"chating to %s", p2p_cxt->peer_name.c_str());
		int j = 0;
		for (int i = p2p_cxt->min; i <= p2p_cxt->max; ++i,++j)
		{
			if(i >= p2p_cxt->msg_list.size())
				continue;
			const auto& it = p2p_cxt->msg_list[i];
			if (it.first == 0)	//self
			{
				wmove(win, y + j, 1);
				attron(COLOR_PAIR(1));
				wprintw(win, "self: %s", it.second.c_str());
				attroff(COLOR_PAIR(1));
			}
			else {				//peer
				wmove(win, y + j, 1);
				attron(COLOR_PAIR(2));
				wprintw(win, "peer: %s", it.second.c_str());
				attroff(COLOR_PAIR(2));
			}	
		}
		wmove(win, p2p_cxt->input_pos, 1);
		wprintw(win, "input: [%s]", p2p_cxt->input_text.c_str());
	}
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
		if (c == 'r' && stream)
			stream->req_peer_list();
		break;
	case ui_state::select_peer:
	{
		if (c >= '0' && c <= '9')
			input_peer_id = (input_peer_id * 10) + (c - '0');
		if (c == KEY_Backspace) //backspace
			input_peer_id = input_peer_id / 10;
		if (c == KEY_Enter) //enter
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
	case ui_state::helper_main:
		if (KEY_ESC == c)
			quit_helper();
		break;
	case ui_state::p2p_main:
		if (KEY_ESC == c)
			quit_p2p();
		break;
	case ui_state::p2p_chat:
	{
		if (KEY_ESC == c)
			quit_p2p();
		else if (c == KEY_Enter && !p2p_cxt->input_text.empty()) //enter
		{
			auto ptr = p2p_stream.lock();
			if (ptr->send_msg(p2p_cxt->input_text))
			{ 
				p2p_cxt->msg_list.push_back({0,std::move(p2p_cxt->input_text)});
				p2p_cxt->scroll_end();
			}
		}
		else if(c > 0)
			p2p_cxt->input_text += c;
	}
		break;
	default:
		break;
	}
	if (c == KEY_ESC)
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

void tui::reg_helper_stream(std::shared_ptr<mqas::tools::p2p::P2PHelperClientStream> stream)
{
	helper_stream = std::move(stream);

	helper_stream->on_change_result.connect(sigc::mem_fun(*this, &tui::on_change_helper_result));
	helper_stream->on_connect_peer.connect(sigc::mem_fun(*this, &tui::on_helper_connect_to));
	helper_stream->on_quit_result.connect(sigc::mem_fun(*this, &tui::on_helper_result));
}

void tui::on_change_helper_result(const std::shared_ptr<mqas::tools::proto::p2p::RespondConnectPeer>& respond)
{
	if (respond->ret() != mqas::tools::proto::p2p::RetCode::ok)
	{
		helper_stream = nullptr;
	}
	else {
		respond_change_helper = std::move(respond);
		if (current_state() == ui_state::get_respond)
			pop_state();
		append_state(ui_state::helper_main);
	}
}
void tui::on_helper_connect_to(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectPeerData>& msg)
{
	try_connect_list.push_back(msg);
}

void tui::on_helper_result(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>& msg,std::shared_ptr<io::UdpSocket> sock)
{
#ifndef NDEBUG
	LOG(DEBUG) << "on_helper_result: " << msg->DebugString();
#endif
	helper_result = msg;
	try_connect_list.clear();
	helper_stream = nullptr;

	if(msg->use_relay())
	{
		launch_relay(helper_result,[this,sock](bool success){
			if(!success)
			{
				clean_relay();
				if (current_state() == ui_state::helper_main)
					pop_state();
				append_state(ui_state::helper_result_failed);
			}else{
				if (current_state() == ui_state::helper_main)
					pop_state();
				append_state(ui_state::p2p_main);
				launch_p2p(helper_result,relay_stream);
			}
		},sock);
		return;
	}

	if (msg->ret() != mqas::tools::proto::p2p::RetCode::ok)
	{
		if (current_state() == ui_state::helper_main)
			pop_state();
		append_state(ui_state::helper_result_failed);
	}
	else {
		if (current_state() == ui_state::helper_main)
			pop_state();
		append_state(ui_state::p2p_main);
		
		launch_p2p(msg,sock);
	}
}


std::optional<mqas::tools::proto::p2p::PeerData> tui::get_peer(uint32_t id) const
{
	if (peer_list == nullptr)
		return {};
	for (int i = 0; i < peer_list->peer_list_size(); ++i)
	{
		auto it = peer_list->peer_list().Get(i);
		if (it.id() == id)
			return it;
	}
	return {};
}

void tui::quit_helper()
{
	if (helper_stream)
	{
		helper_stream->req_quit();
		helper_stream = nullptr;
		try_connect_list.clear();
	}
	clean_relay();
}

void tui::quit_p2p()
{
	if (p2p_engine)
	{
		if(use_relay)
			std::static_pointer_cast<P2PEngineRelayType>(p2p_engine)->close();
		else
			std::static_pointer_cast<P2PEngineType>(p2p_engine)->close();
		on_p2p_peer_quit(nullptr);
	}
}

void tui::on_new_p2p_stream(std::shared_ptr<P2PStreamType> stream, bool is_server)
{
#ifndef NDEBUG
	LOG(DEBUG) << "[on_new_p2p_stream] new p2p stream"
		<< " role=" << (is_server ? "server" : "client")
		<< " stream=" << stream.get();
#endif
	auto self = this;
	std::function<void(std::shared_ptr<P2PChatStream>)> func = [self](std::shared_ptr<P2PChatStream> ptr){
#ifndef NDEBUG
		LOG(DEBUG) << "[on_new_p2p_stream] P2PChatStream ready, ptr=" << ptr.get();
#endif
		self->p2p_stream = ptr;
		ptr->on_received_message.connect(sigc::mem_fun(*self,&tui::on_recive_p2p_msg));
		ptr->on_connected.connect(sigc::mem_fun(*self,&tui::on_p2p_connected));
	};
	if (is_server)
	{
#ifndef NDEBUG
		LOG(DEBUG) << "[on_new_p2p_stream] server: waiting for stream type change to P2PChatStream";
#endif
		stream->on_change_stream_signal.connect([func](std::shared_ptr<core::IStreamVariant> p) {
#ifndef NDEBUG
			LOG(DEBUG) << "[on_new_p2p_stream] server: on_change_stream_signal fired, p=" << p.get();
#endif
			auto ptr = std::dynamic_pointer_cast<P2PChatStream>(p);
			if(ptr)
				func(ptr);
#ifndef NDEBUG
			else
				LOG(DEBUG) << "[on_new_p2p_stream] server: dynamic_pointer_cast to P2PChatStream failed";
#endif
		});
	}
	else {
		test::ReqDirectChat m;
		LOG(INFO) << "req_change p2p chat";
		if (stream->req_change<P2PChatStream, ReqDirectChatPair>(m))
		{
#ifndef NDEBUG
			LOG(DEBUG) << "[on_new_p2p_stream] client: req_change succeeded";
#endif
			auto ptr = stream->get_holds_stream<P2PChatStream>();
			if(ptr)
				func(ptr);
#ifndef NDEBUG
			else
				LOG(DEBUG) << "[on_new_p2p_stream] client: get_holds_stream<P2PChatStream>() returned null";
#endif
		}
#ifndef NDEBUG
		else
			LOG(DEBUG) << "[on_new_p2p_stream] client: req_change failed";
#endif
	}
}

void tui::on_recive_p2p_msg(const std::string& name, const std::string& msg)
{
	p2p_cxt->msg_list.push_back({1,msg});
	//handle sliding window
	p2p_cxt->scroll_end();
}

void tui::on_new_p2p_connect(std::shared_ptr<core::Connect<P2PStreamType>> conn,bool is_server)
{
#ifndef NDEBUG
	LOG(DEBUG) << "[on_new_p2p_connect] p2p connection established"
		<< " role=" << (is_server ? "server" : "client")
		<< " conn=" << conn.get();
#endif
	if (is_server)
	{
#ifndef NDEBUG
		LOG(DEBUG) << "[on_new_p2p_connect] server: waiting for incoming stream";
#endif
		conn->on_new_stream_signal.connect(std::bind(&tui::on_new_p2p_stream, this, std::placeholders::_1, is_server));
	}
	else
	{
#ifndef NDEBUG
		LOG(DEBUG) << "[on_new_p2p_connect] client: making new stream";
#endif
		conn->make_stream(std::bind(&tui::on_new_p2p_stream, this, std::placeholders::_1,is_server));
	}
}

void tui::on_p2p_peer_quit(std::shared_ptr<core::IStreamVariant> stream)
{
	clean_up_p2p(false);
	auto curr = current_state();
	if(curr == ui_state::p2p_chat || curr == ui_state::p2p_main)
		pop_state();
}

void tui::on_p2p_connected(const std::string& name)
{
	if (p2p_server_wait_timer)
		p2p_server_wait_timer->stop();
	if (current_state() == ui_state::p2p_main)
		pop_state();
	append_state(ui_state::p2p_chat);
	
	p2p_cxt = std::make_shared<p2p_chat_cxt>();
	p2p_cxt->height = rows - 1 - 2;
	p2p_cxt->width = cols - 2;
	p2p_cxt->input_pos = rows - 1;
	p2p_cxt->peer_name = name;
}

void tui::clean_up_p2p(bool active)
{
	p2p_stream.reset();
	if(use_relay)
	{
		auto ptr = std::static_pointer_cast<P2PEngineRelayType>(p2p_engine);
		p2p_engine.reset();
	}
	else
	{
		auto ptr = std::static_pointer_cast<P2PEngineType>(p2p_engine);
		p2p_engine.reset();
	}
	p2p_cxt.reset();
	if (p2p_server_wait_timer)
		p2p_server_wait_timer->stop();
}


template<typename SOCK>
requires core::IsVaildSocket<SOCK>
void tui::launch_p2p(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>& msg,std::shared_ptr<SOCK> sock)
{
	using EngineTy = core::sub_engine<core::engine<core::Connect<P2PStreamType>>,core::engine_driver,SOCK>;

	use_relay = std::is_same_v<SOCK,tools::RelayStreamClient>;

	auto io_cxt = comm::locator::inst()->get_ref<io::Context>();
		
	io::Ip::str2addr(msg->peer_addr().ip().c_str(), msg->peer_addr().port(), p2p_addr);

	if constexpr(std::is_same_v<SOCK,tools::RelayStreamClient>)
	{
		sock->get_peer_addr(p2p_addr);
	}

#ifndef NDEBUG
	LOG(DEBUG) << "[launch_p2p]"
		<< " peer_id=" << msg->peer_id()
		<< " is_server=" << msg->is_server()
		<< " use_relay=" << msg->use_relay()
		<< " peer_addr=" << msg->peer_addr().ip() << ":" << msg->peer_addr().port()
		<< " relay_addr=" << msg->relay_addr().ip() << ":" << msg->relay_addr().port()
		<< " white_list_addr=" << io::Ip::addr2str(p2p_addr) << ":" << io::Ip::addr_get_port(p2p_addr)
		<< " reason=" << msg->reason();
#endif

	std::function<void(std::shared_ptr<core::Connect<P2PStreamType>>)> func = std::bind(&tui::on_new_p2p_connect, this, std::placeholders::_1, msg->is_server());
	std::function<void(const std::exception&)> exception_func = [this](const std::exception&) {
		on_p2p_peer_quit(nullptr);
	};
	std::function<void(EngineTy&)> on_init_func = [this](EngineTy& e)
	{
		e.get_engine()->whitelist_addr.push_back(p2p_addr);
		e.get_engine()->whitelist_port.push_back(io::Ip::addr_get_port(p2p_addr));
	};
	auto p2p_conf = toml::find<std::string>(*config,"p2p","conf");
	if (msg->is_server())
	{ 
		p2p_engine = comm::engine_util::launch_sub_engine<P2PStreamType>(io_cxt.value().get(), p2p_conf.c_str(),
			core::EngineFlags::Server, sock, func, nullptr,exception_func, on_init_func);
		if(!p2p_server_wait_timer)
			p2p_server_wait_timer = io_cxt.value().get().make_shared<io::Timer>();
		p2p_server_wait_timer->start([this](io::Timer* t){  
			t->stop();
			quit_p2p();
		},10 * 1000,0);
	}else
	{
		p2p_engine = comm::engine_util::launch_sub_engine<P2PStreamType>(io_cxt.value().get(), p2p_conf.c_str(),
			core::EngineFlags::None, sock, func, &p2p_addr,exception_func, on_init_func);
	}
	std::static_pointer_cast<EngineTy>(p2p_engine)->get_engine()->on_connect_closed_signal.connect([this](std::shared_ptr<core::Connect<P2PStreamType>>) {
		lazy_task.push_back([this]() {
			quit_p2p();
		});
	});
	if(std::is_same_v<SOCK,tools::RelayStreamClient>)
	{
		relay_engine->get_engine()->on_connect_closed_signal.connect([this](std::shared_ptr<core::Connect<RelayStreamType>>){
			lazy_task.push_back([this]() {
				quit_p2p();
			});
		});
	}
}


bool tui::launch_relay(const std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>& msg,std::function<void(bool)> callback,std::shared_ptr<io::UdpSocket> sock)
{
	auto io_cxt = comm::locator::inst()->get_ref<io::Context>();
	
	::sockaddr relay_addr;
	if(!io::Ip::str2addr(msg->relay_addr().ip().c_str(),msg->relay_addr().port(),relay_addr))
	{
		callback(false);
		return false;
	}
	::sockaddr peer_addr;
	if(!io::Ip::str2addr(msg->peer_addr().ip().c_str(),msg->peer_addr().port(),peer_addr))
	{
		callback(false);
		return false;
	}
	tui* ui = this;	
	std::function<void(std::shared_ptr<core::Connect<RelayStreamType>>)> on_connect = [&msg,callback,ui](std::shared_ptr<core::Connect<RelayStreamType>> conn)
	{
		conn->make_stream([&msg,callback,ui](std::shared_ptr<RelayStreamType> stream){
			tools::proto::relay::ReqRelay req;
			auto token = req.mutable_token();
			token->set_data(msg->relay_token());
        	stream->req_change<tools::RelayStreamClient,tools::relay::ReqRelayPair>(req);
			ui->relay_stream = stream->get_holds_stream<tools::RelayStreamClient>();
			ui->relay_stream->on_connect_result.connect([callback](core::StreamVariantErrcode code,std::optional<tools::proto::relay::RespondRelay_Code> ret){
				if(!ret.has_value() || ret.value() != tools::proto::relay::RespondRelay_Code::RespondRelay_Code_success)
					callback(false);
			});
			ui->relay_stream->on_ready.connect([callback](){ callback(true); });
		});
	};

	std::function<void(const std::exception&)> exception_func = [this,callback](const std::exception&) {
		callback(false);
	};
	auto relay_conf = toml::find<std::string>(*config,"relay","conf");
	relay_engine = comm::engine_util::launch_sub_engine<RelayStreamType>(io_cxt.value().get(), relay_conf.c_str(),
			core::EngineFlags::None, sock, on_connect, &relay_addr,exception_func);

	relay_engine->get_engine()->whitelist_addr.push_back(relay_addr);
	relay_engine->get_engine()->whitelist_port.push_back(io::Ip::addr_get_port(relay_addr));
	return true;
}

void tui::clean_relay()
{
	if (relay_engine)
		relay_engine->close();
	if (relay_stream)
		relay_stream.reset();
	if (relay_engine)
		relay_engine.reset();
}
