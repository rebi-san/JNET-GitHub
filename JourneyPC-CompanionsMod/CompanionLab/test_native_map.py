import struct
import unittest
import copy
import io
from unittest.mock import Mock, patch
import native_map
from native_map import offset_for, containing, parse_instruction, runtime_functions, unwind_roots
from test_diagnostic import synthetic_dll
from inspect_build import pe_sections

class NativeMapTests(unittest.TestCase):
    def test_chained_unwind_and_odd_code_padding(self):
        data=synthetic_dll()
        sections=pe_sections(data)
        entries=[(4500,4520,4600),(4520,4540,4620),(4540,4560,4650)]
        def put(rva, value):
            off=offset_for(sections,rva,len(value))
            data[off:off+len(value)]=value
        put(4600,bytes([1,0,0,0]))
        put(4620,bytes([0x21,0,1,0])+bytes(4)+struct.pack('<III',*entries[0]))
        put(4650,bytes([0x21,0,0,0])+struct.pack('<III',*entries[1]))
        self.assertEqual(unwind_roots(data,sections,entries),{4500:4500,4520:4500,4540:4500})
        put(4600,bytes([0x21,0,0,0])+struct.pack('<III',*entries[2]))
        with self.assertRaisesRegex(ValueError,'Cyclic'): unwind_roots(data,sections,entries)
        put(4600,bytes([0x29,0,0,0]))
        with self.assertRaisesRegex(ValueError,'header'): unwind_roots(data,sections,entries)
        put(4600,bytes([1,0,0,0]))
        put(4654,struct.pack('<III',4520,4541,4620))
        with self.assertRaisesRegex(ValueError,'target'): unwind_roots(data,sections,entries)

    def test_ranges_and_gaps(self):
        data=synthetic_dll()
        sections=pe_sections(data)
        struct.pack_into('<II',data,152+112+3*8,4400,24)
        off=offset_for(sections,4400,24)
        struct.pack_into('<6I',data,off,4500,4520,4600,4550,4560,4604)
        ranges=runtime_functions(data,sections)
        starts=[r[0] for r in ranges]
        self.assertEqual(containing(ranges,starts,4500),4500)
        self.assertIsNone(containing(ranges,starts,4520))
        self.assertIsNone(containing(ranges,starts,4400))
        struct.pack_into('<I',data,off+12,4510)
        with self.assertRaises(ValueError): runtime_functions(data,sections)
        with self.assertRaises(ValueError): offset_for(sections,6000,4)

    def test_decoded_edges_only(self):
        base=0x140000000
        call=parse_instruction('140010000:     call\t0x140117730 <.text+0x116730>',base)
        self.assertEqual(call['direct'],0x117730)
        indirect=parse_instruction('140010005: call qword ptr [rax + 0x8]',base)
        self.assertIsNone(indirect['direct'])
        rip=parse_instruction('140010010: lea rcx, [rip + 0x123] # 0x1405cdae0',base)
        self.assertEqual(rip['rip'],0x5cdae0)
        self.assertIsNone(parse_instruction('not an instruction',base))


class NativeMapQueryTests(unittest.TestCase):
    def setUp(self):
        self.data=synthetic_dll()
        sections=pe_sections(self.data)
        self.functions=[(4500,4520,4600),(4520,4540,4620)]
        pe=struct.unpack_from('<I',self.data,60)[0]
        struct.pack_into('<Q',self.data,pe+24+24,0x180000000)
        struct.pack_into('<I',self.data,pe+24+56,8192)
        struct.pack_into('<II',self.data,pe+24+112+3*8,4400,24)
        struct.pack_into('<6I',self.data,offset_for(sections,4400,24),*self.functions[0],*self.functions[1])
        self.data[offset_for(sections,4600,4):offset_for(sections,4600,4)+4]=bytes([1,0,0,0])
        chain=bytes([0x21,0,0,0])+struct.pack('<III',*self.functions[0])
        at=offset_for(sections,4620,len(chain))
        self.data[at:at+len(chain)]=chain
        self.image_size=struct.unpack_from('<I',self.data,pe+24+56)[0]
        self.hash=native_map.digest(self.data)
        self.identity=patch.object(native_map,'EXPECTED_EXE',self.hash)
        self.identity.start()
        self.addCleanup(self.identity.stop)
        first=dict(site=4501,target=4500,owner=4500,fragment=4500,kind='call')
        second=dict(site=4521,target=4500,owner=4500,fragment=4520,kind='jmp')
        leaf=dict(site=4506,target=4550,owner=4500,fragment=4500,kind='call')
        self.report=dict(exe_sha256=self.hash,image_base=struct.unpack_from('<Q',self.data,pe+24+24)[0],
                         runtime_functions=[list(row) for row in self.functions],
                         unwind_roots={'4500':4500,'4520':4500},
                         incoming={'4500':[second,first],'4550':[leaf]},outgoing={'4500':[second,leaf,first]})

    def test_chained_query_is_sorted_and_bounded(self):
        result=native_map.query_cached(self.report,self.data,4530,1)
        self.assertEqual(result['owner'],hex(4500))
        self.assertEqual(result['fragment'],dict(begin=hex(4520),end=hex(4540),unwind=hex(4620)))
        self.assertEqual(result['incoming_to'],hex(4500))
        self.assertEqual(result['incoming']['total'],2)
        self.assertEqual(result['outgoing']['total'],3)
        for direction in ('incoming','outgoing'):
            self.assertTrue(result[direction]['truncated'])
            self.assertEqual(len(result[direction]['edges']),1)
            self.assertEqual(result[direction]['edges'][0]['site'],hex(4501))

    def test_leaf_gap_and_exclusive_range_end(self):
        for at in (0,4540,4550,self.image_size-1):
            result=native_map.query_cached(self.report,self.data,at)
            self.assertIsNone(result['owner'])
            self.assertIsNone(result['fragment'])
            self.assertEqual(result['incoming_to'],hex(at))
            self.assertEqual(result['outgoing']['edges'],[])
        self.assertEqual(native_map.query_cached(self.report,self.data,4550)['incoming']['total'],1)
        self.assertEqual(native_map.query_cached(self.report,self.data,4520)['fragment']['begin'],hex(4520))

    def test_rva_and_limit_bounds(self):
        for at in (-1,self.image_size,0x100000000,True,'4500'):
            with self.subTest(rva=at),self.assertRaises(ValueError):
                native_map.query_cached(self.report,self.data,at)
        for limit in (0,101,-1,True,1.5):
            with self.subTest(limit=limit),self.assertRaises(ValueError):
                native_map.query_cached(self.report,self.data,4500,limit)
        self.assertFalse(native_map.query_cached(self.report,self.data,4500,100)['incoming']['truncated'])

    def test_stale_cache_identity_ranges_and_roots(self):
        for field,value in (('exe_sha256','0'*64),('image_base',0),('runtime_functions',[]),
                            ('unwind_roots',{'4500':4500,'4520':4520}),('incoming',[]),('outgoing',None)):
            bad=copy.deepcopy(self.report);bad[field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):
                native_map.query_cached(bad,self.data,4500)
        changed=bytearray(self.data);changed[-1]^=1
        with self.assertRaisesRegex(ValueError,'Stale'):
            native_map.query_cached(self.report,changed,4500)

    def test_malformed_selected_edges_decline_even_beyond_limit(self):
        for field,value in (('kind','ret'),('site',self.image_size),('target',4502),
                            ('owner',4520),('fragment',4520),('site',True)):
            bad=copy.deepcopy(self.report);bad['incoming']['4500'][1][field]=value
            with self.subTest(field=field,value=value),self.assertRaises(ValueError):
                native_map.query_cached(bad,self.data,4500,1)
        bad=copy.deepcopy(self.report);bad['incoming']['4500']={}
        with self.assertRaises(ValueError): native_map.query_cached(bad,self.data,4500)
        bad=copy.deepcopy(self.report);del bad['incoming']['4500'][1]['owner']
        with self.assertRaises(ValueError): native_map.query_cached(bad,self.data,4500)

    def test_missing_oversized_and_invalid_json_cache_never_rebuild(self):
        cache=Mock();cache.is_file.return_value=False
        with patch.object(native_map,'inside',return_value=cache),patch.object(native_map,'build') as build:
            with self.assertRaisesRegex(ValueError,'missing'): native_map.query(4500)
            cache.is_file.return_value=True;cache.stat.return_value.st_size=64*1024*1024+1
            with self.assertRaisesRegex(ValueError,'limit'): native_map.query(4500)
            cache.stat.return_value.st_size=2;cache.read_text.return_value='{'
            with self.assertRaises(ValueError): native_map.query(4500)
            build.assert_not_called()

    def test_cli_default_build_and_read_only_query(self):
        with patch.object(native_map,'build') as build,patch.object(native_map,'query',return_value={'rva':'0x123'}) as query:
            native_map.main([]);build.assert_called_once_with();query.assert_not_called()
            build.reset_mock()
            with patch('sys.stdout',new_callable=io.StringIO) as out:
                native_map.main(['--at','0x123','--limit','3'])
                self.assertIn('0x123',out.getvalue())
            query.assert_called_once_with(0x123,3);build.assert_not_called()
            with patch('sys.stderr',new_callable=io.StringIO),self.assertRaises(SystemExit):
                native_map.main(['--limit','3'])
            build.assert_not_called()
