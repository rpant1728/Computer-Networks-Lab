// Network topology
//
//       n0 ----------- n1
//            1 Mbps
//             10 ms
//
// Flow from n0 to n1 using BulkSendApplication and 5 constant bit rate sources.
// Measure congestion window size changes, dropped tcp packets and cumulative bytes transferred across tcp variants. 

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
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Compare TCP variants");

// Function passed to tracesource which invokes it each time cwnd changes.
static void CwndTracer(Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval){
	*stream->GetStream() << Simulator::Now().GetSeconds() << "," << newval << std::endl;
}

// Function to attach CwndTracer function to tracesource
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

// Function to extract lost packets from flowMonitor every 0.0001 seconds and print it to file
static void packetDropSample(Ptr<OutputStreamWrapper> stream, Ptr<FlowMonitor> flowMonitor){
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
	uint32_t packetDropCount = stats[1].lostPackets; // stats[1] for the FTP (tcp) flow
  	*stream->GetStream() << Simulator::Now().GetSeconds() << ","  << packetDropCount << std::endl;
	Simulator::Schedule(Seconds(0.0001), &packetDropSample, stream, flowMonitor); // Schedule next sample
}

// Function to calculate cumulative bytes transferred every 0.0001 seconds and print it to file
static void TxTotalBytesSample(Ptr<OutputStreamWrapper> stream, std::vector <Ptr<PacketSink>> sinks){
	uint32_t totalBytes = 0;
	for (uint32_t it = 0; it < 6; it++){ // Add bytesTx of 1 ftp app + 5 cbr udp apps 
		totalBytes += sinks[it]->GetTotalRx(); 
	}
	*stream->GetStream() << Simulator::Now().GetSeconds() << ","  << totalBytes << std::endl;
	Simulator::Schedule(Seconds(0.0001), &TxTotalBytesSample, stream, sinks);
}

int main(int argc, char *argv[]){
	uint32_t maxBytes = 0;
	std::string transport_prot = "TCPNewReno";
	std::string cwndTrFileName = "Cwnd.tr";
    std::string pktDropFileName = "PktDrop.tr";
    std::string bytesTxFileName = "BytesTx.tr";

	// Allow the user to override any of the defaults at run-time, via command-line arguments
	CommandLine cmd;
	cmd.AddValue("transport_prot", "Transport protocol to use: TcpNewReno, TcpHybla, TcpVegas, TcpScalable, TcpWestwood", transport_prot);
	cmd.Parse(argc, argv);

	NS_LOG_INFO("Create nodes.");
	NodeContainer nodes;
	nodes.Create(2);

	NS_LOG_INFO("Create channels."); 
	// LogComponentEnable ("BulkSendApplication", LOG_LEVEL_ALL); 
    // LogComponentEnable ("OnOffApplication", LOG_LEVEL_ALL); 

	// Explicitly create the point-to-point link required by the topology(shown above).
	PointToPointHelper pointToPoint;
	pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

	NetDeviceContainer devices;
	devices = pointToPoint.Install(nodes);

	//Setting TCP protocol using argument passed in the command line
	if(transport_prot.compare("TcpNewReno") == 0){ 
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
	}
	else if(transport_prot.compare("TcpHybla") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
	}
	else if(transport_prot.compare("TcpWestwood") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
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

	// Install the internet stack on the nodes
	InternetStackHelper internet;
	internet.Install(nodes);

	TrafficControlHelper tch;
  	tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue ("5p"));
	QueueDiscContainer qdiscs = tch.Install (devices);
	
	// We've got the "hardware" in place. Now we need to add IP addresses.
	NS_LOG_INFO("Assign IP Addresses.");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(devices);

	NS_LOG_INFO("Create Applications.");

	uint16_t port = 50000;
	BulkSendHelper source("ns3::TcpSocketFactory",	InetSocketAddress(i.GetAddress(1), port));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
	ApplicationContainer sourceApps = source.Install(nodes.Get(0));
	sourceApps.Start(MilliSeconds(0));
	sourceApps.Stop(MilliSeconds(1800));

	// Create a PacketSinkApplication and install it on node 1
	PacketSinkHelper sink("ns3::TcpSocketFactory",	InetSocketAddress(Ipv4Address::GetAny(), port));
	ApplicationContainer sinkApp = sink.Install(nodes.Get(1));
	sinkApp.Start(MilliSeconds(0));
	sinkApp.Stop(MilliSeconds(1800));

	// Add 5 constant bit rate sources
	uint16_t cbr_port = 10;
	std::vector<int16_t> start_times{200, 400, 600, 800, 1000};
	std::vector<int16_t>  stop_times{1800, 1800, 1200, 1400, 1600};
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

	// Now, do the actual simulation.
	NS_LOG_INFO("Run Simulation.");
	
    // Enable congestion window sampler
    Simulator::Schedule(Seconds(0.00001), &TraceCwnd, transport_prot+cwndTrFileName);
	
    
	Ptr<FlowMonitor> flowMonitor;
	FlowMonitorHelper flowHelper;
	flowMonitor = flowHelper.InstallAll();

	AsciiTraceHelper ascii;
	Ptr<OutputStreamWrapper> packetDropStream = ascii.CreateFileStream(transport_prot + pktDropFileName);
	// Schedule first instance of sampler function, then it will schedule itself
	Simulator::Schedule(Seconds(0.00001), &packetDropSample, packetDropStream, flowMonitor);

	// Schedule first tx bytes sample function pass its file stream 
	Ptr<OutputStreamWrapper> TxTotalByteStream = ascii.CreateFileStream(transport_prot + bytesTxFileName);
	Simulator::Schedule(Seconds(0.00001), &TxTotalBytesSample, TxTotalByteStream, sinks);
	
	Simulator::Stop(MilliSeconds(1800));
	Simulator::Run();
	
	// Create a flow monitor to obtains stats for monitoting data values
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

	// Print data values using flow-monitor
	std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
	std::cout << "  Tx Packets/Bytes:   " << stats[1].txPackets
			<< " / " << stats[1].txBytes << std::endl;
	std::cout << "  Offered Load: " << stats[1].txBytes * 8.0 / (stats[1].timeLastTxPacket.GetSeconds () - stats[1].timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
	std::cout << "  Rx Packets/Bytes:   " << stats[1].rxPackets
			<< " / " << stats[1].rxBytes << std::endl;
	uint32_t packetsDroppedByQueueDisc = 0;
	uint64_t bytesDroppedByQueueDisc = 0;
	if (stats[1].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE_DISC)
	{
		std::cout << stats[1].packetsDropped.size () << std::endl;
		packetsDroppedByQueueDisc = stats[1].packetsDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
		bytesDroppedByQueueDisc = stats[1].bytesDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
	}
	std::cout << "  Packets/Bytes Dropped by Queue Disc:   " << packetsDroppedByQueueDisc
			<< " / " << bytesDroppedByQueueDisc << std::endl;
	uint32_t packetsDroppedByNetDevice = 0;
	uint64_t bytesDroppedByNetDevice = 0;
	if (stats[1].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE)
	{
		std::cout << stats[1].packetsDropped.size () << std::endl;
		packetsDroppedByNetDevice = stats[1].packetsDropped[Ipv4FlowProbe::DROP_QUEUE];
		bytesDroppedByNetDevice = stats[1].bytesDropped[Ipv4FlowProbe::DROP_QUEUE];
	}
	std::cout << "  Packets/Bytes Dropped by NetDevice:   " << packetsDroppedByNetDevice
			<< " / " << bytesDroppedByNetDevice << std::endl;
	std::cout << "  Throughput: " << stats[1].rxBytes * 8.0 / (stats[1].timeLastRxPacket.GetSeconds () - stats[1].timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
	std::cout << "  Mean delay:   " << stats[1].delaySum.GetSeconds () / stats[1].rxPackets << std::endl;
	std::cout << "  Mean jitter:   " << stats[1].jitterSum.GetSeconds () / (stats[1].rxPackets - 1) << std::endl;

  	for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i){
		std::cout << " --------------------------------- " << std::endl;
		std::cout << "Flow Id: " << i->first << std::endl;
		std::cout << "Tx Bytes: " << i->second.txBytes  << std::endl;
		std::cout << "Drop Packet Count: " << i->second.lostPackets << std::endl;
	}

	Simulator::Destroy();
	NS_LOG_INFO("Done.");
}
