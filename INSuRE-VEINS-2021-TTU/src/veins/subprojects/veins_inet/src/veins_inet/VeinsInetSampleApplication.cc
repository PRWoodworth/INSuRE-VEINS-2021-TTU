//
// Copyright (C) 2018 Christoph Sommer <sommer@ccs-labs.org>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "veins_inet/VeinsInetSampleApplication.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "veins_inet/VeinsInetSampleMessage_m.h"

using namespace inet;

Define_Module(VeinsInetSampleApplication);

VeinsInetSampleApplication::VeinsInetSampleApplication()
{
}

bool VeinsInetSampleApplication::startApplication()
{
//    traci->addVehicle(4, vtype0, route0, 0, 0, 14, 2);
//    traci->addVehicle(4, vtype0, route0, 0, 0, 14, 3);
    // host[0] should stop at t=20s
    if (getParentModule()->getIndex() == 0) {
        auto callback = [this]() {
            getParentModule()->getDisplayString().setTagArg("i", 1, "red");
//            traciVehicle->setSpeed(0);

            auto payload = makeShared<VeinsInetSampleMessage>();
            timestampPayload(payload);
            payload->setChunkLength(B(100));
            payload->setRoadId(traciVehicle->getRoadId().c_str());
            //TODO
            payload->setMessageType("runaway");

            auto packet = createPacket("maliciousPacket");
            packet->insertAtBack(payload);
            sendPacket(std::move(packet));

            // host should continue after 30s
            auto callback = [this]() {
                traciVehicle->setSpeed(-1);
            };
            timerManager.create(veins::TimerSpecification(callback).oneshotIn(SimTime(30, SIMTIME_S)));
        };
        timerManager.create(veins::TimerSpecification(callback).oneshotAt(SimTime(20, SIMTIME_S)));
    }

    return true;
}

bool VeinsInetSampleApplication::stopApplication()
{
    return true;
}

VeinsInetSampleApplication::~VeinsInetSampleApplication()
{
}

void VeinsInetSampleApplication::processPacket(std::shared_ptr<inet::Packet> pk)
{
    auto payload = pk->peekAtFront<VeinsInetSampleMessage>();
    //TODO
    auto message_type = payload->getMessageType();


    EV_INFO << "Received packet: " << payload << endl;

    getParentModule()->getDisplayString().setTagArg("i", 1, "green");

    //Ensure only vehicles following node0 (first vehicle) react.
    if (getParentModule()->getIndex() != 0) {
        if(strcmp(message_type, "accident_occurred") == 0){
            //Original code - reroutes all vehicles due to an accident.
            traciVehicle->changeRoute(payload->getRoadId(), 999.9);
        }else if(strcmp(message_type, "brakes_on") == 0){
            //Activate brakes.
            traciVehicle->slowDown(0, 20);
        }else if(strcmp(message_type, "object_front") == 0){
            //Slow down and change lanes, since there is supposedly an object infront of the vehicle.
            traciVehicle->setSpeed(traciVehicle->getSpeed()/2);
            traciVehicle->changeLane(1, 20.00);
        }else if(strcmp(message_type, "runaway") == 0){
            //Set the acceleration to something higher.
            traciVehicle->setSpeedMode(0);
            traciVehicle->setSpeed(traciVehicle->getSpeed()*3);
        }
    }
    auto callback = [this]() {
        traciVehicle->setSpeed(20);
    };
    timerManager.create(veins::TimerSpecification(callback).oneshotIn(SimTime(10, SIMTIME_S)));
    if (haveForwarded) return;

    auto packet = createPacket("relay");
    packet->insertAtBack(payload);
    sendPacket(std::move(packet));

    haveForwarded = true;
}
