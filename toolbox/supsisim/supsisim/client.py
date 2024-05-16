#!/usr/bin/env python3
import asyncio
from pydoc import locate
from threading import Thread

from shv import RpcUrl, SimpleClient, RpcError, SHVType


def _start_background_loop(loop: asyncio.AbstractEventLoop) -> None:
    asyncio.set_event_loop(loop)
    try:
        loop.run_forever()
    finally:
        print("Ending connection loop...")
        for task in asyncio.all_tasks():
            task.cancel()
        
        print("Remaining tasks canceled...")
    loop.close()
    print("Ending connection loop, all Done")

async def _disconnect_client(client: SimpleClient):
    if await _is_connected(client):
        await client.disconnect()

async def _connect_client(user: str, addr: str, port: str, password: str) -> SimpleClient | None:
    url: RpcUrl = RpcUrl.parse("tcp://{}@{}:{}?password={}".format(user, addr, port, password))

    print("Connection to SHV broker: ", url.to_url())
    return await SimpleClient.connect(url)


async def _get_parameter_value(client: SimpleClient, mount_point:str, device_id: str, item: str, paramName: str) -> SHVType|None:
    callUrl = "{}/{}/blocks/{}/parameters/{}".format(mount_point, device_id, paramName, item)
    try:
        result = await client.call(callUrl, "get") 
        return result
    except RpcError as e:
        print("Can't read parameter ", paramName)
        print(e)
        return None

async def _set_parameter_value(client: SimpleClient, mount_point:str, device_id: str, item: str, paramName: str, paramVal: SHVType):
    callUrl = "{}/{}/blocks/{}/parameters/{}".format(mount_point, device_id, paramName, item)
    # print("Set param url: ", callUrl)
    try:
        return await client.call(callUrl, "set", paramVal) 
    except RpcError as e:
        print("Can't set parameter ", paramName)
        print(e)

async def _is_connected(client: SimpleClient) -> bool:
    return client.client.connected()


class BrokerConnection:
    def __init__(self):
        self.asyncio_loop = asyncio.new_event_loop()
        self.asyncio_thread = Thread(target=_start_background_loop, args=(self.asyncio_loop,), daemon=True)
        self.client: SimpleClient | None = None
        self.addr: str | None = None
        self.port: str | None = None
        self.user: str | None = None
        self.password: str | None = None
        self.device_id: str | None = None
        self.mount_point: str | None = None

        self.asyncio_thread.start()

    def __del__(self):
        self.asyncio_loop.stop()
        
    
    def _connect(self):
        print("Connecting to broker...")
        try:
            client = asyncio.run_coroutine_threadsafe(_connect_client(self.user, self.addr, self.port, self.password), self.asyncio_loop).result()
        except:
            print("No connection to brocker")
            return

        if client is None:
            print("No connection to brocker")
        

        self.client = client

        print("Connected to broker.")

    def _disconnect(self):
        if self.client is None:
            return
        print("Disconnecting from broker...")
        asyncio.run_coroutine_threadsafe(_disconnect_client(self.client), self.asyncio_loop).result()

        self.client = None
        
        print("Disconnected from broker.")

    def _get_connection(self):
        if self.client is None or not asyncio.run_coroutine_threadsafe(_is_connected(self.client), self.asyncio_loop).result():
            self._connect()            

        return self.client

    def update_parameters_and_connect(self, addr: str, port: str, user: str, password: str, device_id: str, mount_point: str):

        are_values_updated = (self.addr != addr or 
             self.port != port or 
             self.user != user or 
             self.password != password or 
             self.device_id != device_id or
             self.mount_point != mount_point)

        
        self.addr = addr
        self.port = port
        self.user = user
        self.password = password
        self.device_id = device_id
        self.mount_point = mount_point

        if are_values_updated:
            self._disconnect()

        if self.client is None:
            self._connect()
    
    def isConnected(self)-> bool:
        if self.client is None:
            return False
        
        return asyncio.run_coroutine_threadsafe(_is_connected(self.client), self.asyncio_loop).result()


    def getParameterValue(self, item: str, paramName: str) -> SHVType|None:
        client = self._get_connection()
        return asyncio.run_coroutine_threadsafe(
            _get_parameter_value(client, self.mount_point, self.device_id, item, paramName), 
            self.asyncio_loop
            ).result()

    def setPrameterValue(self, item: str, paramName: str, paramVal: SHVType):
        client = self._get_connection()
        asyncio.run_coroutine_threadsafe(
            _set_parameter_value(client, self.mount_point, self.device_id, item, paramName, paramVal), 
            self.asyncio_loop
            ).result()
        
    def disconnect(self):
        self._disconnect()
