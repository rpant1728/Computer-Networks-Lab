/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Network topology
//
//       n0 ----------- n1
//            1 Mbps
//             10 ms
//
// - Flow from n0 to n1 using BulkSendApplication.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

uint32_t txBytesum = 0;
uint32_t TxDropCount = 0;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");

static void CwndTracer(Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval){
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << newval << std::endl;
}

static void TraceCwnd(std::string cwndTrFileName){
    AsciiTraceHelper ascii;
    if(cwndTrFileName.compare("") == 0){
        NS_LOG_DEBUG("No trace file for cwnd provided");
        return;
    }
    else{
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(cwndTrFileName.c_str());
        Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndTracer, stream));
    }
}

static void TxDropSample(Ptr<OutputStreamWrapper> stream){
  	*stream->GetStream() << Simulator::Now().GetSeconds() << ","  << TxDropCount << std::endl;
	Simulator::Schedule(Seconds(0.001), &TxDropSample, stream);
}

static void TxDropIncrement(Ptr<PcapFileWrapper> file, Ptr<const Packet> p){
  	NS_LOG_UNCOND("TxDrop at " << Simulator::Now().GetSeconds());
	file->Write (Simulator::Now (), p);
	TxDropCount++;
}

static void TxTotalBytesSample(Ptr<OutputStreamWrapper> stream, std:: vector <Ptr<PacketSink>> sinks){
	uint32_t totalBytes = 0;
	for (uint32_t it = 0; it < 6; it++){
		totalBytes += sinks[it]->GetTotalRx();
	}
	*stream->GetStream() << Simulator::Now().GetSeconds() << ","  << totalBytes << std::endl;
	Simulator::Schedule(Seconds(0.001), &TxTotalBytesSample, stream, sinks);
}

int main(int argc, char *argv[]){
	bool tracing = false;
	std::string transport_prot = "TCPNewReno";
	uint32_t maxBytes = 0;
  uint32_t pktSize = 1458;        //in bytes. 1458 to prevent fragments
	std::string cwndTrFileName = "cwndTCPVariant.tr";

	// Allow the user to override any of the defaults at
	// run-time, via command-line arguments

	CommandLine cmd;
	cmd.AddValue("transport_prot", "Transport protocol to use: TcpNewReno, TcpHybla, TcpVegas, TcpScalable, TcpWestwood", transport_prot);
  cmd.AddValue("cwndTrFileName", "Name of cwnd trace file", cwndTrFileName);
	cmd.Parse(argc, argv);

	NS_LOG_INFO("Create nodes.");
	NodeContainer nodes;
	nodes.Create(2);

	NS_LOG_INFO("Create channels.");
  	// LogComponentEnable("BulkSendApplication", LOG_LEVEL_ALL); 
  	// LogComponentEnable("OnOffApplication", LOG_LEVEL_ALL); 
	// Explicitly create the point-to-point link required by the topology(shown above).

	PointToPointHelper pointToPoint;
	pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("10000p"));
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

	NetDeviceContainer devices;
	devices = pointToPoint.Install(nodes);

	if(transport_prot.compare("TcpNewReno") == 0){ 
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
	}
	else if(transport_prot.compare("TcpHybla") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
	}
	else if(transport_prot.compare("TcpWestwood") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));  // TODO
		// Config::SetDefault("ns3::TcpWestwood::ProtocolType", EnumValue(TcpWestwood::WESTWOODPLUS));
	}
	else if(transport_prot.compare("TcpScalable") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
	}
	else if(transport_prot.compare("TcpVegas") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));
	}
	else {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
	}

  // Set error rate to cover losses other than buffer loss
	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
	em->SetAttribute("ErrorRate", DoubleValue(0.00001));
	devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

	// Install the internet stack on the nodes
	InternetStackHelper internet;
	internet.Install(nodes);

	// We've got the "hardware" in place. Now we need to add IP addresses.
	NS_LOG_INFO("Assign IP Addresses.");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(devices);

	NS_LOG_INFO("Create Applications.");

	// Create a BulkSendApplication and install it on node 0
	// uint16_t port = 9;  // well-known echo port number
	uint16_t port = 12345;  // well-known echo port number


	BulkSendHelper source("ns3::TcpSocketFactory",	InetSocketAddress(i.GetAddress(1), port));
	// Set the amount of data to send in bytes.  Zero is unlimited.
  	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktSize));
	source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
  	source.SetAttribute("SendSize", UintegerValue(pktSize));
	ApplicationContainer sourceApps = source.Install(nodes.Get(0));
	sourceApps.Start(MilliSeconds(0));
	sourceApps.Stop(MilliSeconds(18000));

	// Create a PacketSinkApplication and install it on node 1
	PacketSinkHelper sink("ns3::TcpSocketFactory",	InetSocketAddress(Ipv4Address::GetAny(), port));
	ApplicationContainer sinkApp = sink.Install(nodes.Get(1));
	sinkApp.Start(MilliSeconds(0));
	sinkApp.Stop(MilliSeconds(18000));

	// Add 5 constant bit rate sources
	uint16_t cbr_port = 10;   // Discard port(RFC 863)
	std::vector<int16_t> start_times{200, 400, 600, 800, 1000};
	std::vector<int16_t>  stop_times{18000, 18000, 12000, 14000, 16000};
	std::vector<Ptr<PacketSink>> sinks(6);

	sinks[0] = DynamicCast<PacketSink>(sinkApp.Get(0));

	for(uint16_t it = 0; it < 5; it++) {
    // Create ith constant bit source
		OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(0), cbr_port+it)));
		onoff.SetConstantRate(DataRate("300Kbps"));
		ApplicationContainer apps = onoff.Install(nodes.Get(0));
		apps.Start(MilliSeconds(start_times[it]));
		apps.Stop(MilliSeconds(stop_times[it]));

		// Create ith packet sink to receive above packets
		PacketSinkHelper sink("ns3::UdpSocketFactory",	Address(InetSocketAddress(Ipv4Address::GetAny(), cbr_port+it)));
		apps = sink.Install(nodes.Get(1));
		apps.Start(MilliSeconds(start_times[it]));
		apps.Stop(MilliSeconds(stop_times[it]));  
	
		sinks[it+1] = DynamicCast<PacketSink>(apps.Get(0));
	}

	AsciiTraceHelper ascii;

	// Set up tracing if enabled
	if(tracing){
		pointToPoint.EnableAsciiAll(ascii.CreateFileStream("tcp-bulk-send.tr"));
		pointToPoint.EnablePcapAll("tcp-bulk-send", false);
	}
	
	// Now, do the actual simulation.
	NS_LOG_INFO("Run Simulation.");
	Simulator::Schedule(Seconds(0.00001), &TraceCwnd, cwndTrFileName);
	
	// Subscribe to packets dropped at transmission
	// Ptr<BulkSendApplication> app = DynamicCast<BulkSendApplication> (nodes.Get(0)->GetApplication(0));
	// Ptr<PacketSink> app = DynamicCast<PacketSink> (nodes.Get(1)->GetApplication(0));	
	// devices.Get(1)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&TxDropIncrement));
	// devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&TxDropIncrement, file));
	// app->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&TxDropIncrement));

	PcapHelper pcapHelper;
	Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", std::ios::out, PcapHelper::DLT_PPP);
	devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&TxDropIncrement, file));


	Ptr<OutputStreamWrapper> TxDropStream = ascii.CreateFileStream("tx-drop-samples.tr");
	// Schedule first instance of sampler function, then it will schedule itself
	Simulator::Schedule(Seconds(0.001), &TxDropSample, TxDropStream);

	// todo
	Ptr<OutputStreamWrapper> TxTotalByteStream = ascii.CreateFileStream("tx-byte-samples.tr");
	Simulator::Schedule(Seconds(0.001), &TxTotalBytesSample, TxTotalByteStream , sinks);
	
	Ptr<FlowMonitor> flowMonitor;
	FlowMonitorHelper flowHelper;
	flowMonitor = flowHelper.InstallAll();
	
	Simulator::Stop(MilliSeconds(18000));
	Simulator::Run();
	
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
  	for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i){
		// txBytesum+=i->second.txBytes;
		// dropPackets+= i->second.lostPackets;
		// std::cout << "For FlowID : "<< stats[0].flowId << std::endl;
		std::cout << " --------------------------------- " << std::endl;
		std::cout << "All Tx Bytes: " << i->second.txBytes  << std::endl;
		std::cout << "All Drop Packets: " << i->second.lostPackets << std::endl;
	}

	flowMonitor->SerializeToXmlFile("abc.xml", true, true);

	Simulator::Destroy();
	NS_LOG_INFO("Done.");

	Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApp.Get(0));
	std::cout << "Total Bytes Received: " << sink1->GetTotalRx() << std::endl;
}
