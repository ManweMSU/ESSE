#include "CommUnix.h"
#include <Cor/CorVirtualMemory.h>

#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>

namespace ESSE
{
	namespace Communicatio
	{
		void SetPosixError(int error, ErrorContext & ectx) noexcept
		{
			if (error == 0) ErrorSet(ectx, 0x08, 0x00);
			else if (error == EADDRINUSE) ErrorSet(ectx, 0x08, 0x02);
			else if (error == EADDRNOTAVAIL) ErrorSet(ectx, 0x08, 0x03);
			else if (error == ECONNREFUSED) ErrorSet(ectx, 0x08, 0x07);
			else if (error == EHOSTUNREACH) ErrorSet(ectx, 0x08, 0x05);
			else if (error == ENETDOWN) ErrorSet(ectx, 0x08, 0x04);
			else if (error == ENETUNREACH) ErrorSet(ectx, 0x08, 0x06);
			else if (error == EPROTOTYPE) ErrorSet(ectx, 0x08, 0x09);
			else if (error == ETIMEDOUT) ErrorSet(ectx, 0x08, 0x0A);
			else if (error == ECONNRESET) ErrorSet(ectx, 0x08, 0x08);
			else if (error == EACCES) ErrorSet(ectx, 0x06, 5);
			else if (error == EDQUOT) ErrorSet(ectx, 0x06, 10);
			else if (error == EEXIST) ErrorSet(ectx, 0x06, 11);
			else if (error == EISDIR) ErrorSet(ectx, 0x06, 11);
			else if (error == EMFILE) ErrorSet(ectx, 0x06, 4);
			else if (error == ENAMETOOLONG) ErrorSet(ectx, 0x06, 17);
			else if (error == ENFILE) ErrorSet(ectx, 0x06, 10);
			else if (error == ENOENT) ErrorSet(ectx, 0x06, 2);
			else if (error == ENOSPC) ErrorSet(ectx, 0x06, 10);
			else if (error == ENOTDIR) ErrorSet(ectx, 0x06, 3);
			else if (error == EOPNOTSUPP) ErrorSet(ectx, 0x06, 12);
			else if (error == EROFS) ErrorSet(ectx, 0x06, 9);
			else if (error == ETXTBSY) ErrorSet(ectx, 0x06, 5);
			else if (error == EILSEQ) ErrorSet(ectx, 0x06, 16);
			else if (error == EBADF) ErrorSet(ectx, 0x06, 6);
			else if (error == EINVAL) ErrorSet(ectx, 0x06, 12);
			else if (error == ENOTEMPTY) ErrorSet(ectx, 0x06, 13);
			else if (error == ENOTDIR) ErrorSet(ectx, 0x06, 3);
			else if (error == ENOTSUP) ErrorSet(ectx, 0x06, 12);
			else if (error == EPERM) ErrorSet(ectx, 0x06, 5);
			else if (error == EXDEV) ErrorSet(ectx, 0x06, 15);
			else if (error == ENOBUFS) ErrorSet(ectx, 0x06, 7);
			else if (error == ENOMEM) ErrorSet(ectx, 0x06, 7);
			else if (error == ENXIO) ErrorSet(ectx, 0x06, 8);
			else if (error == EFBIG) ErrorSet(ectx, 0x06, 18);
			else if (error == EBUSY) ErrorSet(ectx, 0x06, 5);
			else if (error == EISCONN) ErrorSet(ectx, 0x05, 0);
			else if (error == ESHUTDOWN) ErrorSet(ectx, 0x05, 0);
			else ErrorSet(ectx, 0x08, 0x01);
		}
		void SetPosixError(ErrorContext & ectx) noexcept { SetPosixError(errno, ectx); }
		void SetDNSError(int error, ErrorContext & ectx) noexcept
		{
			if (error) {
				if (error == EAI_ADDRFAMILY || error == EAI_FAMILY || error == EAI_NODATA || error == EAI_NONAME || error == EAI_SERVICE || error == EAI_SOCKTYPE) ErrorSet(ectx, 0x08, 0x0C);
				else if (error == EAI_AGAIN || error == EAI_FAIL) ErrorSet(ectx, 5, 0);
				else if (error == EAI_BADFLAGS) ErrorSet(ectx, 3, 0);
				else if (error == EAI_MEMORY || error == EAI_OVERFLOW) ErrorSet(ectx, 2, 0);
				else if (error == EAI_SYSTEM) SetPosixError(ectx);
				else ErrorSet(ectx, 0x08, 0x01);
			} else ErrorSet(ectx, 0, 0);
		}

		void SocketAddressInit(DataBlock & dest, NetworkAddress * address, ucs1_string * ulnk)
		{
			if (address) {
				if (address->GetDomain() == NetworkAddressDomain::Local) {
					auto & addr = *reinterpret_cast<NetworkAddressLocal *>(address);
					dest.SetLength(sizeof(sockaddr_un));
					auto & sa = *reinterpret_cast<sockaddr_un *>(dest.GetBuffer());
					sa.sun_family = PF_LOCAL;
					Memory::ZeroMemory(&sa.sun_path, sizeof(sa.sun_path));
					ucs1_string path;
					if (addr.name[0] == L'/') path = addr.name; else path = U"/tmp/" + addr.name;
					if (ulnk) *ulnk = path;
					Memory::MemoryCopy(&sa.sun_path, path.GetData(), min(path.GetLength(), uintptr(sizeof(sa.sun_path))));
				} else if (address->GetDomain() == NetworkAddressDomain::IPv4) {
					auto & addr = *reinterpret_cast<NetworkAddressIPv4 *>(address);
					dest.SetLength(sizeof(sockaddr_in));
					auto & sa = *reinterpret_cast<sockaddr_in *>(dest.GetBuffer());
					sa.sin_family = AF_INET;
					sa.sin_port = Memory::ReverseByteOrder(addr.port);
					Memory::MemoryCopy(&sa.sin_addr, &addr.address, 4);
					Memory::ZeroMemory(&sa.sin_zero, sizeof(sa.sin_zero));
				} else if (address->GetDomain() == NetworkAddressDomain::IPv6) {
					auto & addr = *reinterpret_cast<NetworkAddressIPv6 *>(address);
					dest.SetLength(sizeof(sockaddr_in6));
					auto & sa = *reinterpret_cast<sockaddr_in6 *>(dest.GetBuffer());
					sa.sin6_family = AF_INET6;
					sa.sin6_port = Memory::ReverseByteOrder(addr.port);
					sa.sin6_flowinfo = 0;
					Memory::MemoryCopy(&sa.sin6_addr, &addr.address, 16);
					for (int i = 0; i < 16; i += 2) swap(sa.sin6_addr.s6_addr[i], sa.sin6_addr.s6_addr[i + 1]);
					sa.sin6_scope_id = 0;
				} else throw Exception();
			} else throw InvalidArgumentException();
		}
		void SocketAddressRead(void * src, oref<NetworkAddress> * dest)
		{
			auto & sa = *reinterpret_cast<const sockaddr *>(src);
			if (sa.sa_family == PF_LOCAL) {
				auto & sa_local = *reinterpret_cast<const sockaddr_un *>(src);
				auto address = owrap(new NetworkAddressLocal(sa_local.sun_path));
				*dest = address.Inner();
			} else if (sa.sa_family == PF_INET) {
				auto & sa_ipv4 = *reinterpret_cast<const sockaddr_in *>(src);
				auto address = owrap(new NetworkAddressIPv4(0, 0, 0, 0, 0));
				Memory::MemoryCopy(&address->address, &sa_ipv4.sin_addr, 4);
				address->port = Memory::ReverseByteOrder(sa_ipv4.sin_port);
				*dest = address.Inner();
			} else if (sa.sa_family == PF_INET6) {
				auto & sa_ipv6 = *reinterpret_cast<sockaddr_in6 *>(src);
				auto address = owrap(new NetworkAddressIPv6(0, 0, 0, 0, 0, 0, 0, 0, 0));
				for (int i = 0; i < 16; i += 2) swap(sa_ipv6.sin6_addr.s6_addr[i], sa_ipv6.sin6_addr.s6_addr[i + 1]);
				Memory::MemoryCopy(&address->address, &sa_ipv6.sin6_addr, 16);
				address->port = Memory::ReverseByteOrder(sa_ipv6.sin6_port);
				*dest = address.Inner();
			} else throw InvalidStateException();
		}

		class NetworkRequestQueue : public Object
		{
			static oref<Thread> _dispatch_thread;
			static int _command_in;
			static int _command_out;
			static uint _service_refcnt;
		private:
			int _socket, _address_size;
			ucs1_string _unlink_on_close;
			oref<Semaphore> _local_sync;
			Queue<NetworkRequest> _requests_read;
			Queue<NetworkRequest> _requests_write;
			oref<IOpenSSLEventHandler> _ossl_hdlr;
		private:
			typedef Dictionary<oref<NetworkRequestQueue>, uint> QueueList;
			typedef KeyValuePair<oref<NetworkRequestQueue>, uint> QueueItem;
			static uint _evaluate_poll_attribute(NetworkRequest * rqread, NetworkRequest * rqwrite) noexcept
			{
				uint result = 0;
				if (rqread && (rqread->in.attributes & REQUEST_ATTRIBUTE_PRIORITY)) result |= rqread->in.attributes & REQUEST_ATTRIBUTE_POLL_MASK;
				else if (rqwrite && (rqwrite->in.attributes & REQUEST_ATTRIBUTE_PRIORITY)) result |= rqwrite->in.attributes & REQUEST_ATTRIBUTE_POLL_MASK;
				else {
					if (rqread) result |= rqread->in.attributes & REQUEST_ATTRIBUTE_POLL_MASK;
					if (rqwrite) result |= rqwrite->in.attributes & REQUEST_ATTRIBUTE_POLL_MASK;
				}
				return result;
			}
			static bool _handle_connection(int pollstat, QueueItem & item, uint attribute, int & remove_list) noexcept
			{
				bool revise = false;
				oref<IDispatchTask> hdlr;
				item.key->_local_sync->Wait();
				Queue<NetworkRequest> * queue = 0;
				auto read_req_ptr = item.key->_requests_read.GetFirst();
				auto write_req_ptr = item.key->_requests_write.GetFirst();
				Queue<NetworkRequest>::Element * req_ptr;
				if (read_req_ptr && attribute == REQUEST_ATTRIBUTE_POLL_READ && !(read_req_ptr->GetValue().in.attributes & REQUEST_ATTRIBUTE_OPENSSL)) { req_ptr = read_req_ptr; queue = &item.key->_requests_read; }
				else if (write_req_ptr && attribute == REQUEST_ATTRIBUTE_POLL_WRITE && !(write_req_ptr->GetValue().in.attributes & REQUEST_ATTRIBUTE_OPENSSL)) { req_ptr = write_req_ptr; queue = &item.key->_requests_write; }
				else req_ptr = 0;
				if (req_ptr) try {
					auto & req = req_ptr->GetValue();
					if (req.in.attributes & REQUEST_ATTRIBUTE_CONNECT_BIT) {
						bool finish;
						ErrorContext error;
						int status = connect(item.key->_socket, reinterpret_cast<sockaddr *>(req.in.data->GetBuffer()), req.in.data->GetLength());
						if (status == 0) {
							finish = true;
							ErrorClear(error);
						} else if (status == -1) {
							if (errno == EINTR || errno == EAGAIN || errno == EINPROGRESS) {
								finish = false;
							} else if (errno == EISCONN) {
								finish = true;
								ErrorClear(error);
							} else {
								finish = true;
								SetPosixError(error);
							}
						} else {
							finish = true;
							ErrorSet(error, 5, 0);
						}
						if (finish) {
							if (req.out.status) *req.out.status = error;
							hdlr = req.out.handler;
							queue->RemoveFirst();
							revise = true;
						}
					} else if (req.in.attributes & REQUEST_ATTRIBUTE_SEND_BIT) {
						bool finish;
						ErrorContext error;
						if (pollstat & POLLHUP) {
							finish = true;
							ErrorSet(error, 0x08, 0x08);
						} else {
							int rem = req.in.length - req.in.pointer;
							int status = send(item.key->_socket, req.in.data->GetBuffer() + req.in.pointer, rem, 0);
							if (status >= 0) {
								req.in.pointer += status;
								if (req.in.pointer == req.in.length) {
									finish = true;
									ErrorClear(error);
								} else finish = false;
							} else {
								if (errno == EAGAIN || errno == EINTR) {
									finish = false;
								} else {
									finish = true;
									SetPosixError(error);
								}
							}
						}
						if (finish) {
							if (req.out.status) *req.out.status = error;
							if (req.out.length) *req.out.length = req.in.pointer;
							hdlr = req.out.handler;
							queue->RemoveFirst();
							revise = true;
						}
					} else if (req.in.attributes & REQUEST_ATTRIBUTE_RECEIVE_BIT) {
						bool finish;
						ErrorContext error;
						int rem = req.in.length - req.in.pointer;
						int status = recv(item.key->_socket, req.in.data->GetBuffer() + req.in.pointer, rem, 0);
						if (status > 0) {
							req.in.pointer += status;
							if (req.in.pointer == req.in.length) {
								finish = true;
								ErrorClear(error);
							} else finish = false;
						} else if (status == 0) {
							finish = true;
							ErrorClear(error);
						} else {
							if (errno == EAGAIN || errno == EINTR) {
								finish = false;
							} else {
								finish = true;
								SetPosixError(error);
							}
						}
						if (finish) {
							req.in.data->SetLength(req.in.pointer);
							if (req.out.status) *req.out.status = error;
							if (req.out.data) *req.out.data = req.in.data;
							hdlr = req.out.handler;
							queue->RemoveFirst();
							revise = true;
						}
					} else if (req.in.attributes & REQUEST_ATTRIBUTE_ACCEPT_BIT) {
						bool process;
						uint8 sa[0x100];
						ErrorContext error;
						socklen_t length = sizeof(sa);
						int socket_accepted = accept(item.key->_socket, reinterpret_cast<sockaddr *>(&sa), &length);
						if (socket_accepted == -1) {
							if (errno == EINTR || errno == EWOULDBLOCK) {
								process = false;
							} else {
								process = true;
								SetPosixError(error);
							}
						} else {
							process = true;
							ErrorClear(error);
						}
						if (process) {
							NewNetworkConnection con;
							con.in_socket = socket_accepted;
							con.in_address = &sa;
							con.in_address_length = length;
							if (socket_accepted >= 0) _allocate_connection(con, error);
							if (req.out.status) *req.out.status = error;
							if (req.out.channel) *req.out.channel = con.out_channel;
							if (req.out.address) *req.out.address = con.out_address;
							hdlr = req.out.handler;
							if (req.in.pointer > 0) {
								req.in.pointer--;
								if (!req.in.pointer) {
									queue->RemoveFirst();
									revise = true;
								}
							}
						}
					} else throw InvalidStateException();
				} catch (...) {
					item.key->_local_sync->Open();
					return false;
				} else if (item.key->_ossl_hdlr) {
					auto status = item.key->_ossl_hdlr->ProcessConnection(pollstat,
						read_req_ptr && (read_req_ptr->GetValue().in.attributes & REQUEST_ATTRIBUTE_OPENSSL) ? &read_req_ptr->GetValue() : 0,
						write_req_ptr && (write_req_ptr->GetValue().in.attributes & REQUEST_ATTRIBUTE_OPENSSL) ? &write_req_ptr->GetValue() : 0, attribute);
					if (status == OpenSSLEventStatus::PollRenew) {
						revise = true;
					} else if (status == OpenSSLEventStatus::CompleteReadRequest) {
						hdlr = read_req_ptr->GetValue().out.handler;
						item.key->_requests_read.RemoveFirst();
						revise = true;
					} else if (status == OpenSSLEventStatus::CompleteWriteRequest) {
						hdlr = write_req_ptr->GetValue().out.handler;
						item.key->_requests_write.RemoveFirst();
						revise = true;
					} else if (status == OpenSSLEventStatus::Failed) {
						item.key->_local_sync->Open();
						return false;
					} else if (status == OpenSSLEventStatus::CloseChannelWrite) {
						hdlr = write_req_ptr->GetValue().out.handler;
						ErrorContext ectx;
						ErrorClear(ectx);
						item.key->ShutdownRequest(false, ectx);
						item.key->_requests_write.RemoveFirst();
						revise = true;
					} else if (status == OpenSSLEventStatus::CloseChannel) {
						hdlr = write_req_ptr->GetValue().out.handler;
						ErrorContext ectx;
						ErrorClear(ectx);
						item.key->ShutdownRequest(true, ectx);
						item.key->_requests_write.RemoveFirst();
						revise = true;
					}
				}
				item.key->_local_sync->Open();
				if (hdlr) hdlr->DoTask(0);
				if (revise) {
					auto next_rd = item.key->_peek_request(REQUEST_ATTRIBUTE_POLL_READ);
					auto next_wr = item.key->_peek_request(REQUEST_ATTRIBUTE_POLL_WRITE);
					if (next_rd || next_wr) {
						item.value = _evaluate_poll_attribute(next_rd, next_wr);
					} else {
						item.value = REQUEST_ATTRIBUTE_REMOVE;
						remove_list++;
					}
				}
				return true;
			}
			static void _allocate_connection(NewNetworkConnection & con, ErrorContext & ectx) noexcept;
			static int _dispatch(void * arg) noexcept
			{
				try {
					QueueList clients; // client : poll attribute
					array<pollfd> poll_data(0x100);
					int poll_volume = 1;
					uint resize_counter = 0;
					while (true) {
						if (poll_volume != poll_data.GetLength()) {
							resize_counter++;
							if (poll_volume > poll_data.GetLength() || !(resize_counter & 0x1FF)) poll_data.SetLength(poll_volume);
						}
						poll_data[0].fd = _command_out;
						poll_data[0].events = POLLIN;
						poll_data[0].revents = 0;
						int i = 1;
						for (auto & c : clients) {
							poll_data[i].fd = c.key->_peek_handle();
							poll_data[i].events = poll_data[i].revents = 0;
							if (c.value & REQUEST_ATTRIBUTE_POLL_READ) poll_data[i].events |= POLLIN;
							if (c.value & REQUEST_ATTRIBUTE_POLL_WRITE) poll_data[i].events |= POLLOUT;
							i++;
						}
						int status = poll(poll_data, poll_volume, -1);
						if (status > 0) {
							int remove_list = 0;
							i = 1;
							for (auto & c : clients) {
								if ((poll_data[i].revents & POLLERR) || (poll_data[i].revents & POLLHUP) || (poll_data[i].revents & POLLIN)) {
									if (!_handle_connection(poll_data[i].revents, c, REQUEST_ATTRIBUTE_POLL_READ, remove_list)) break;
								}
								if ((poll_data[i].revents & POLLERR) || (poll_data[i].revents & POLLHUP) || (poll_data[i].revents & POLLOUT)) {
									if (!_handle_connection(poll_data[i].revents, c, REQUEST_ATTRIBUTE_POLL_WRITE, remove_list)) break;
								}
								i++;
							}
							if (remove_list) {
								auto current = clients.GetFirst();
								while (current) {
									auto next = current->GetNext();
									if (current->GetValue().value & REQUEST_ATTRIBUTE_REMOVE) { clients.BinaryTree::Remove(current); poll_volume--; }
									current = next;
								}
							}
							if (poll_data[0].revents) {
								uintptr command[2];
								int rd = 0;
								while (rd < sizeof(command)) {
									int nr = read(_command_out, reinterpret_cast<char *>(&command) + rd, sizeof(command) - rd);
									if (nr > 0) rd += nr;
									else if (nr == 0) { Memory::ZeroMemory(&command, sizeof(command)); break; }
									else {
										if (errno == EINTR || errno == EAGAIN) continue;
										else break;
									}
								}
								if (!command[1]) break;
								auto queue = owrap(reinterpret_cast<NetworkRequestQueue *>(command[1]));
								if (command[0]) {
									bool created;
									auto rd_rq = queue->_peek_request(REQUEST_ATTRIBUTE_POLL_READ);
									auto wr_rq = queue->_peek_request(REQUEST_ATTRIBUTE_POLL_WRITE);
									if (rd_rq || wr_rq) {
										auto element = clients.FindElement(QueueItem(queue, 0), true, &created);
										if (created) poll_volume++;
										element->GetValue().value = _evaluate_poll_attribute(rd_rq, wr_rq);
									} else command[0] = 0;
								}
								if (!command[0]) {
									auto element = clients.FindElementEquivalent(queue);
									if (element) {
										clients.BinaryTree::Remove(element);
										poll_volume--;
									}
								}
							}
						} else if (status == -1) { if (errno == EINVAL) break; }
					}
				} catch (...) {}
				close(_command_out);
				return 0;
			}
			int _peek_handle(void) noexcept { return _socket; }
			NetworkRequest * _peek_request(uint attribute) noexcept
			{
				NetworkRequest * result = 0;
				_local_sync->Wait();
				Queue<NetworkRequest>::Element * first = 0;
				if ((attribute & REQUEST_ATTRIBUTE_POLL_MASK) == REQUEST_ATTRIBUTE_POLL_READ) first = _requests_read.GetFirst();
				else if ((attribute & REQUEST_ATTRIBUTE_POLL_MASK) == REQUEST_ATTRIBUTE_POLL_WRITE) first = _requests_write.GetFirst();
				if (first) result = &first->GetValue();
				_local_sync->Open();
				return result;
			}
			bool _control_send(uintptr cmd) noexcept
			{
				Memory::AcquireRootLock();
				if (_command_in < 0) {
					Memory::ReleaseRootLock();
					return false;
				}
				uintptr command[2];
				command[0] = cmd;
				command[1] = uintptr(this);
				int length = sizeof(command);
				int wp = 0;
				while (wp < length) {
					int wa = write(_command_in, &command, length - wp);
					if (wa == -1) {
						auto errid = errno;
						if (errid == EINTR) continue; else {
							Memory::ReleaseRootLock();
							return false;
						}
					} else wp += wa;
				}
				Retain();
				Memory::ReleaseRootLock();
				return true;
			}
			void _common_init(void) { CreateDispatch(); _local_sync = CreateSemaphore(1); if (!_local_sync) throw OutOfMemoryException(); }
		public:
			NetworkRequestQueue(int sock) : _socket(sock), _address_size(0)
			{
				if (fcntl(_socket, F_SETFL, O_NONBLOCK) == -1) {
					ErrorContext ectx;
					SetPosixError(ectx);
					ErrorThrow(ectx);
				}
				_common_init();
			}
			NetworkRequestQueue(NetworkAddress * address) : _address_size(0)
			{
				if (address) {
					if (address->GetDomain() == NetworkAddressDomain::Local) {
						_socket = socket(PF_LOCAL, SOCK_STREAM, 0);
					} else if (address->GetDomain() == NetworkAddressDomain::IPv4) {
						_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
					} else if (address->GetDomain() == NetworkAddressDomain::IPv6) {
						_socket = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
					} else throw Exception();
				} else throw InvalidArgumentException();
				if (_socket == -1) {
					ErrorContext ectx;
					SetPosixError(ectx);
					ErrorThrow(ectx);
				}
				if (fcntl(_socket, F_SETFL, O_NONBLOCK) == -1) {
					ErrorContext ectx;
					SetPosixError(ectx);
					close(_socket);
					ErrorThrow(ectx);
				}
				_common_init();
			}
			virtual ~NetworkRequestQueue(void)
			{
				for (auto & r : _requests_read) {
					if (r.out.status) ErrorSet(*r.out.status, 0x08, 0x08);
					if (r.out.handler) r.out.handler->DoTask(0);
				}
				for (auto & r : _requests_write) {
					if (r.out.status) ErrorSet(*r.out.status, 0x08, 0x08);
					if (r.out.handler) r.out.handler->DoTask(0);
				}
				close(_socket);
				if (_unlink_on_close.GetLength()) unlink(_unlink_on_close);
				StopDispatch();
			}
			void BindRequest(NetworkAddress * address, ErrorContext & ectx) noexcept
			{
				ESSE_TRY_INTRO
					ucs1_string ulnk;
					DataBlock sa(1);
					SocketAddressInit(sa, address, &ulnk);
					auto status = bind(_socket, reinterpret_cast<sockaddr *>(sa.GetBuffer()), sa.GetLength());
					if (status == -1) { SetPosixError(ectx); return; }
					status = listen(_socket, SOMAXCONN);
					if (status == -1) { SetPosixError(ectx); return; }
					_address_size = sa.GetLength();
					_unlink_on_close = ucs1_string();
					if (ulnk.GetLength()) try { _unlink_on_close = ulnk; } catch (...) {}
				ESSE_TRY_OUTRO()
			}
			void ShutdownRequest(bool bidirectionally, ErrorContext & ectx) noexcept
			{
				int status;
				if (bidirectionally) status = shutdown(_socket, SHUT_RDWR);
				else status = shutdown(_socket, SHUT_WR);
			}
			void EnqueueConnectRequest(NetworkAddress * address, ErrorContext * error, IDispatchTask * hdlr)
			{
				_local_sync->Wait();
				try {
					NetworkRequest req;
					req.in.attributes = REQUEST_ATTRIBUTE_CONNECT;
					req.in.length = req.in.pointer = 0;
					req.in.data = new DataBlock(1);
					req.out.handler.SetRetain(hdlr);
					req.out.status = error;
					req.out.length = 0;
					req.out.data = 0;
					req.out.address = 0;
					req.out.channel = 0;
					SocketAddressInit(*req.in.data, address, 0);
					while (true) {
						if (connect(_socket, reinterpret_cast<sockaddr *>(req.in.data->GetBuffer()), req.in.data->GetLength()) == -1) {
							auto errid = errno;
							if (errid == EINPROGRESS) {
								_requests_write.Push(req);
								if (!_control_send(1)) {
									_requests_write.RemoveLast();
									_local_sync->Open();
									if (req.out.status) ErrorSet(*req.out.status, 0x8, 0x4);
									if (req.out.handler) req.out.handler->DoTask(0);
									return;
								}
								break;
							} else if (errid == EINTR) {
								continue;
							} else {
								_local_sync->Open();
								if (req.out.status) SetPosixError(errid, *req.out.status);
								if (req.out.handler) req.out.handler->DoTask(0);
								return;
							}
						} else {
							_local_sync->Open();
							if (req.out.status) ErrorClear(*req.out.status);
							if (req.out.handler) req.out.handler->DoTask(0);
							return;
						}
					}
				} catch (...) {
					_local_sync->Open();
					throw;
				}
				_local_sync->Open();
			}
			void EnqueueRequest(const NetworkRequest & req)
			{
				_local_sync->Wait();
				try {
					Queue<NetworkRequest> * queue = 0;
					if (req.in.attributes & REQUEST_ATTRIBUTE_POLL_READ) queue = &_requests_read;
					if (req.in.attributes & REQUEST_ATTRIBUTE_POLL_WRITE) queue = &_requests_write;
					if (!queue) throw InvalidArgumentException();
					queue->Push(req);
					if (!_control_send(1)) {
						queue->RemoveLast();
						_local_sync->Open();
						if (req.out.status) ErrorSet(*req.out.status, 0x8, 0x4);
						if (req.out.handler) req.out.handler->DoTask(0);
						return;
					}
				} catch (...) {
					_local_sync->Open();
					throw;
				}
				_local_sync->Open();
			}
			void Shutdown(void) noexcept { _control_send(0); }
			static void StopDispatch(void) noexcept
			{
				if (InterlockedDecrement(_service_refcnt) == 0) {
					Memory::AcquireRootLock();
					close(_command_in);
					_command_in = -1;
					Memory::ReleaseRootLock();
					_dispatch_thread->Wait();
				}
			}
			static void CreateDispatch(void)
			{
				if (InterlockedIncrement(_service_refcnt) == 1) {
					try {
						int p[2];
						if (pipe(p) == -1) throw OutOfMemoryException();
						_command_in = p[1];
						_command_out = p[0];
						if (fcntl(_command_out, F_SETFL, O_NONBLOCK) == -1) { close(_command_in); close(_command_out); throw OutOfMemoryException(); }
						_dispatch_thread = CreateThread(_dispatch);
						if (!_dispatch_thread) { close(_command_in); close(_command_out); throw OutOfMemoryException(); }
					} catch (...) {
						InterlockedDecrement(_service_refcnt);
						throw;
					}
				}
			}
			int GetSocket(void) noexcept { return _socket; }
			Semaphore * GetSynchronize(void) noexcept { return _local_sync; }
			void SetOpenSSLEventHandler(IOpenSSLEventHandler * hdlr) noexcept { _ossl_hdlr.SetRetain(hdlr); }
		};
		class UnixNetworkChannel : public INetworkChannel
		{
			oref<NetworkRequestQueue> _queue;
		public:
			UnixNetworkChannel(void) {}
			UnixNetworkChannel(NetworkRequestQueue * queue) { _queue.SetRetain(queue); }
			virtual ~UnixNetworkChannel(void) noexcept { if (_queue) _queue->Shutdown(); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Unix Channel"; ESSE_TRY_OUTRO(string()) }
			virtual void Connect(NetworkAddress * address, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (_queue) { ErrorSet(ectx, 5, 0); return; }
				ESSE_TRY_INTRO
					_queue = owrap(new NetworkRequestQueue(address));
					_queue->EnqueueConnectRequest(address, error, hdlr);
				ESSE_TRY_OUTRO()
			}
			virtual void Connect(NetworkAddress * address, NetworkSecurityDesc & sec, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override { ErrorSet(ectx, 1, 0); }
			virtual void Send(DataBlock * data, ErrorContext * error, uintptr * sent, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (!_queue) { ErrorSet(ectx, 5, 0); return; }
				if (!data || !data->GetLength()) {
					if (error) ErrorClear(*error);
					if (sent) *sent = 0;
					if (hdlr) hdlr->DoTask(0);
					return;
				}
				ESSE_TRY_INTRO
					NetworkRequest req;
					req.in.attributes = REQUEST_ATTRIBUTE_SEND;
					req.in.length = data->GetLength();
					req.in.pointer = 0;
					req.in.data.SetRetain(data);
					req.out.handler.SetRetain(hdlr);
					req.out.status = error;
					req.out.length = sent;
					req.out.data = 0;
					req.out.address = 0;
					req.out.channel = 0;
					_queue->EnqueueRequest(req);
				ESSE_TRY_OUTRO()
			}
			virtual void Receive(uintptr length, ErrorContext * error, oref<DataBlock> * data, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (!_queue) { ErrorSet(ectx, 5, 0); return; }
				if (length < 0) { ErrorSet(ectx, 3, 0); return; }
				ESSE_TRY_INTRO
					auto responce = owrap(new DataBlock(1));
					if (!length) {
						if (error) ErrorClear(*error);
						if (data) *data = responce;
						if (hdlr) hdlr->DoTask(0);
						return;
					}
					responce->SetLength(length);
					NetworkRequest req;
					req.in.attributes = REQUEST_ATTRIBUTE_RECEIVE;
					req.in.length = length;
					req.in.pointer = 0;
					req.in.data = responce;
					req.out.handler.SetRetain(hdlr);
					req.out.status = error;
					req.out.length = 0;
					req.out.data = data;
					req.out.address = 0;
					req.out.channel = 0;
					_queue->EnqueueRequest(req);
				ESSE_TRY_OUTRO()
			}
			virtual void Close(bool ultimately, ErrorContext & ectx) noexcept override
			{
				if (!_queue) { ErrorSet(ectx, 5, 0); return; }
				_queue->ShutdownRequest(ultimately, ectx);
			}
			NetworkRequestQueue * GetQueue(void) noexcept { return _queue; }
		};
		class UnixNetworkListener : public INetworkListener
		{
			oref<NetworkRequestQueue> _queue;
		public:
			UnixNetworkListener(void) {}
			virtual ~UnixNetworkListener(void) noexcept { if (_queue) _queue->Shutdown(); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Unix Listener"; ESSE_TRY_OUTRO(string()) }
			virtual void Bind(NetworkAddress * address, ErrorContext & ectx) noexcept override
			{
				if (_queue) { ErrorSet(ectx, 5, 0); return; }
				ESSE_TRY_INTRO
					_queue = owrap(new NetworkRequestQueue(address));
					_queue->BindRequest(address, ectx);
				ESSE_TRY_OUTRO()
			}
			virtual void Bind(NetworkAddress * address, NetworkIdentityDesc & idesc, ErrorContext & ectx) noexcept override { ErrorSet(ectx, 1, 0); }
			virtual void Accept(uintptr limit, ErrorContext * error, oref<INetworkChannel> * channel, oref<NetworkAddress> * address, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (!_queue) { ErrorSet(ectx, 5, 0); return; }
				ESSE_TRY_INTRO
					NetworkRequest req;
					req.in.attributes = REQUEST_ATTRIBUTE_ACCEPT;
					req.in.length = 0;
					req.in.pointer = limit > 0 ? limit : -1;
					req.out.handler.SetRetain(hdlr);
					req.out.status = error;
					req.out.length = 0;
					req.out.data = 0;
					req.out.address = address;
					req.out.channel = channel;
					_queue->EnqueueRequest(req);
				ESSE_TRY_OUTRO()
			}
			virtual void Close(ErrorContext & ectx) noexcept override
			{
				if (!_queue) { ErrorSet(ectx, 5, 0); return; }
				_queue->ShutdownRequest(true, ectx);
			}
		};

		oref<Thread> NetworkRequestQueue::_dispatch_thread;
		int NetworkRequestQueue::_command_in = -1;
		int NetworkRequestQueue::_command_out = -1;
		uint NetworkRequestQueue::_service_refcnt = 0;

		void NetworkRequestQueue::_allocate_connection(NewNetworkConnection & con, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				oref<NetworkRequestQueue> queue;
				try { queue = owrap(new NetworkRequestQueue(con.in_socket)); } catch (...) { close(con.in_socket); throw; }
				try {
					con.out_channel = new UnixNetworkChannel(queue);
					SocketAddressRead(con.in_address, &con.out_address);
				} catch (...) {
					con.out_channel.Clear();
					con.out_address.Clear();
					throw;
				}
			ESSE_TRY_OUTRO()
		}

		void NetworkEngineInit(void) { NetworkRequestQueue::CreateDispatch(); }
		void NetworkEngineStop(void) noexcept { NetworkRequestQueue::StopDispatch(); }
		oref<INetworkChannel> CreateNetworkChannel(NetworkAddress * address) { return new UnixNetworkChannel; }
		oref<INetworkListener> CreateNetworkListener(NetworkAddress * address) { return new UnixNetworkListener; }
		oref<object_array<NetworkAddress>> GetNetworkDomainAddresses(const string & domain_name, uint16 port, NetworkAddressDomain address_domain, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				addrinfo * info = 0, * current = 0, req;
				Memory::ZeroMemory(&req, sizeof(req));
				req.ai_flags = AI_CANONNAME;
				req.ai_protocol = IPPROTO_TCP;
				if (address_domain == NetworkAddressDomain::IPv4) {
					req.ai_family = AF_INET;
				} else if (address_domain == NetworkAddressDomain::IPv6) {
					req.ai_family = PF_INET6;
					req.ai_flags |= AI_ALL | AI_V4MAPPED;
				} else throw NotImplementedException();
				ucs1_string ascii_domain = ConvertDomainToPunycode(domain_name);
				ucs1_string ascii_port = string(port);
				auto error = getaddrinfo(ascii_domain, ascii_port, &req, &info);
				if (error) { SetDNSError(error, ectx); return 0; }
				oref<object_array<NetworkAddress>> result;
				try {
					result = owrap(new object_array<NetworkAddress>(0x10));
					current = info;
					while (current) {
						oref<NetworkAddress> addr;
						SocketAddressRead(current->ai_addr, &addr);
						result->Append(addr);
						current = current->ai_next;
					}
				} catch (...) { freeaddrinfo(info); throw; }
				freeaddrinfo(info);
				return result;
			ESSE_TRY_OUTRO(0)
		}

		int UnixChannelGetSocket(INetworkChannel * channel) noexcept
		{
			auto queue = static_cast<UnixNetworkChannel *>(channel)->GetQueue();
			return queue ? queue->GetSocket() : -1;
		}
		void UnixChannelEnqueueRequest(INetworkChannel * channel, const NetworkRequest & req)
		{
			auto queue = static_cast<UnixNetworkChannel *>(channel)->GetQueue();
			if (!queue) throw InvalidStateException();
			queue->EnqueueRequest(req);
		}
		void UnixChannelEnterCriticalSection(INetworkChannel * channel) noexcept
		{
			auto queue = static_cast<UnixNetworkChannel *>(channel)->GetQueue();
			if (queue) queue->GetSynchronize()->Wait();
		}
		void UnixChannelLeaveCriticalSection(INetworkChannel * channel) noexcept
		{
			auto queue = static_cast<UnixNetworkChannel *>(channel)->GetQueue();
			if (queue) queue->GetSynchronize()->Open();
		}
		void UnixChannelSetOpenSSLEventHandler(INetworkChannel * channel, IOpenSSLEventHandler * hdlr) noexcept
		{
			auto queue = static_cast<UnixNetworkChannel *>(channel)->GetQueue();
			if (queue) queue->SetOpenSSLEventHandler(hdlr);
		}
	}
}