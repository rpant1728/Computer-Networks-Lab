/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
//            500 Kbps
//             5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

static void CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval){
    // *stream->GetStream () << oldval << "," << newval << std::endl;
    std::cout << oldval << "," << newval << std::endl;

}

static void TraceCwnd (std::string cwndTrFileName){
    AsciiTraceHelper ascii;
    if (cwndTrFileName.compare ("") == 0){
        NS_LOG_DEBUG ("No trace file for cwnd provided");
        return;
    }
    else{
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (cwndTrFileName.c_str ());
        Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",MakeBoundCallback (&CwndTracer, stream));
    }
}

int
main (int argc, char *argv[])
{

  bool tracing = false;
  uint32_t maxBytes = 0;
    std::string cwndTrFileName = "cwndTCPVariant.tr";

  // CommandLine cmd;
    //    cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, "
    //                 "TcpHybla, TcpVegas, TcpScalable, TcpWestwood", transport_prot);
    //    cmd.AddValue ("cwndTrFileName", "Name of cwnd trace file", cwndTrFileName);
    // cmd.Parse (argc, argv);
 

//
// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.Parse (argc, argv);

//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (2);

  NS_LOG_INFO ("Create channels.");

//
// Explicitly create the point-to-point link required by the topology (shown above).
//
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  // if (transport_prot.compare ("ns3::TcpNewReno") == 0){ 
  //       Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  //   }
  //   else if(transport_prot.compare ("ns3::TcpHybla") == 0){
  //       Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHybla::GetTypeId ()));
  //   }
  //   else if(transport_prot.compare ("ns3::TcpWestwood") == 0){
  //       Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
  //   }
  //   else if(transport_prot.compare ("ns3::TcpScalable") == 0){
  //       Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpScalable::GetTypeId ()));
  //   }
  //   else if(transport_prot.compare ("ns3::TcpVegas") == 0){
  //       Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
  //   }
  //   else {
  //       Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  //   }

//
// Install the internet stack on the nodes
//
  InternetStackHelper internet;
  internet.Install (nodes);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  NS_LOG_INFO ("Create Applications.");

//
// Create a BulkSendApplication and install it on node 0
//
  uint16_t port = 9;  // well-known echo port number


  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0));
  sourceApps.Stop (Seconds (1));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0));
  sinkApps.Stop (Seconds (1));

 uint16_t port2 = 10;   // Discard port (RFC 863)
  OnOffHelper onoff2 ("ns3::UdpSocketFactory", 
                      Address (InetSocketAddress (i.GetAddress (0), port2)));
  onoff2.SetConstantRate (DataRate ("2Mbps"));
  ApplicationContainer apps2 = onoff2.Install (nodes.Get (0));
  apps2.Start (Seconds (0));
  apps2.Stop (Seconds (1));

  // Create a packet sink to receive these packets
  PacketSinkHelper sink2 ("ns3::UdpSocketFactory",
                          Address (InetSocketAddress (Ipv4Address::GetAny (), port2)));
  apps2 = sink2.Install (nodes.Get (1));
  apps2.Start (Seconds (0));
  apps2.Stop (Seconds (1));

//
// Set up tracing if enabled
//
  if (tracing)
    {
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      pointToPoint.EnablePcapAll ("tcp-bulk-send", false);
    }

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Schedule(Seconds(0.00001), &TraceCwnd, cwndTrFileName);
  Simulator::Stop (Seconds (1));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}

