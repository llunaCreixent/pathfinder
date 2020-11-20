#include "binaryExporter.h"

#include "encoding.h"
#include "exceptions.h"

using namespace std;

BinaryExporter::BinaryExporter(string const& _file):
	m_file(_file)
{
}

void BinaryExporter::write(DB const& _db)
{
	writeAddresses(_db);

	write(_db.safes);
	write(_db.tokens);
	// TODO the limit has to be a percentage
	write(_db.connections);
}

void BinaryExporter::write(set<Edge> const& _edges)
{
	writeAddresses(_edges);

	cout << "Exporting " << _edges.size() << " edges and " << m_addresses.size() << " unique addresses." << endl;

	for (Edge const& edge: _edges)
	{
		write(edge.from);
		write(edge.to);
		write(edge.token);
		write(edge.capacity);
	}
}


void BinaryExporter::write(size_t const& _size)
{
	require(_size < numeric_limits<uint32_t>::max());
	m_file << BigEndian<4>(_size);
}

void BinaryExporter::write(Address const& _address)
{
	write(indexOf(_address));
}

void BinaryExporter::write(Int const& _v)
{
	bool wroteLength = false;
	for (int i = 31; i >= 0; --i)
	{
		uint64_t data = (_v.data[i / 8] >> ((i * 8) % 64)) & 0xff;
		if (!wroteLength && (i == 0 || data != 0))
		{
			m_file.put(char(size_t(i + 1)));
			wroteLength = true;
		}
		if (wroteLength)
			m_file.put(char(data));
	}
}

void BinaryExporter::write(Safe const& _safe)
{
	write(_safe.address);
	write(_safe.balances);
}

void BinaryExporter::write(Token const& _token)
{
	write(_token.address);
	write(_token.safeAddress);
}

void BinaryExporter::write(Connection const& _connection)
{
	write(_connection.canSendToAddress);
	write(_connection.userAddress);
	write(_connection.limit);
}

void BinaryExporter::writeAddresses(DB const& _db)
{
	set<Address> addresses;
	for (Safe const& safe: _db.safes)
	{
		addresses.insert(safe.address);
		for (auto const& balance: safe.balances)
			addresses.insert(balance.first);
	}
	for (Token const& token: _db.tokens)
	{
		addresses.insert(token.address);
		addresses.insert(token.safeAddress);
	}
	writeAddresses(addresses);
}

void BinaryExporter::writeAddresses(set<Edge> const& _edges)
{
	set<Address> addresses;
	for (Edge const& edge: _edges)
	{
		addresses.insert(edge.from);
		addresses.insert(edge.to);
		addresses.insert(edge.token);
	}
	writeAddresses(addresses);
}

void BinaryExporter::writeAddresses(set<Address> const& _addresses)
{
	require(m_addresses.empty());
	m_addresses = {_addresses.begin(), _addresses.end()};

	write(m_addresses.size());
	for (auto const& address: m_addresses)
		m_file.write(reinterpret_cast<char const*>(&(address.address[0])), 20);
}

size_t BinaryExporter::indexOf(Address const& _address)
{
	auto it = lower_bound(m_addresses.begin(), m_addresses.end(), _address);
	require(it != m_addresses.end());
	return size_t(it - m_addresses.begin());
}