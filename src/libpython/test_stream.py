import unittest
import os
from os import path as PyPath
from mitsuba import DummyStream, FileStream, MemoryStream
from mitsuba.filesystem import path

def touch(path):
    # Equivalent of `touch` that is compatible with Python 2
    open(str(path), 'a').close()

class CommonStreamTest(unittest.TestCase):
    roPath = path('./read_only_file_for_common_tests')
    woPath = path('./write_only_file_for_common_tests')

    # TODO: more contents, exercise lots of types
    contents = ['some sentence', 42, 13.37]

    def writeContents(self, stream):
        for v in CommonStreamTest.contents:
            stream.writeValue(v)
        stream.flush()

    def setUp(self):
        touch(CommonStreamTest.roPath)
        # Provide a fresh instances of each kind of Stream implementation
        self.streams = {}
        self.streams['DummyStream'] = DummyStream()
        self.streams['MemoryStream'] = MemoryStream(64)
        self.streams['FileStream (read only)'] = FileStream(CommonStreamTest.roPath, False)
        self.streams['DummyStream (write only)'] = FileStream(CommonStreamTest.woPath, True)

    def tearDown(self):
        os.remove(str(CommonStreamTest.roPath))
        os.remove(str(CommonStreamTest.woPath))

    def test01_size_and_pos(self):
        for (name, stream) in self.streams.items():
            with self.subTest(name):
                self.assertEqual(stream.getSize(), 0)
                self.assertEqual(stream.getPos(), 0)

                if stream.canWrite():
                    # string length as a uint32_t (4) + string (5)
                    stream.writeValue("hello")
                    stream.flush()
                    self.assertEqual(stream.getSize(), 9)
                    self.assertEqual(stream.getPos(), 9)
                    stream.writeValue(42) # int (1)
                    stream.flush()
                    self.assertEqual(stream.getSize(), 9+1)
                    self.assertEqual(stream.getPos(), 9+1)

    def test02_truncate(self):
        for (name, stream) in self.streams.items():
            if not stream.canWrite():
                continue
            with self.subTest(name):
                self.assertEqual(stream.getSize(), 0)
                self.assertEqual(stream.getPos(), 0)
                stream.truncate(100)
                self.assertEqual(stream.getSize(), 100)
                self.assertEqual(stream.getPos(), 0)
                stream.seek(99)
                self.assertEqual(stream.getPos(), 99)
                stream.truncate(50)
                self.assertEqual(stream.getSize(), 50)
                self.assertEqual(stream.getPos(), 50)
                stream.writeValue("hello")
                stream.flush()
                self.assertEqual(stream.getSize(), 50 + 9)
                self.assertEqual(stream.getPos(), 50 + 9)

    def test03_seek(self):
        for (name, stream) in self.streams.items():
            with self.subTest(name):
                size = 0

                self.assertEqual(stream.getSize(), size)
                self.assertEqual(stream.getPos(), 0)

                if stream.canWrite():
                    size = 5
                    stream.truncate(size)
                    self.assertEqual(stream.getSize(), size)
                    self.assertEqual(stream.getPos(), 0)

                stream.seek(5)
                self.assertEqual(stream.getSize(), size)
                self.assertEqual(stream.getPos(), 5)
                # Seeking beyond the end of the file is okay, but won't make it larger
                # TODO: this behavior is inconsistent for MemoryStream
                stream.seek(20)
                self.assertEqual(stream.getSize(), size)
                self.assertEqual(stream.getPos(), 20)

                if stream.canWrite():
                    # A subsequent write should start at the correct position
                    # and update the size.
                    stream.writeValue(13.37) # Float (1)
                    self.assertEqual(stream.getPos(), 20 + 1)
                    self.assertEqual(stream.getSize(), 20 + 1)

    def test04_read_back(self):
        # Write some values to be read back
        temporaryWriteStream = FileStream(CommonStreamTest.roPath, True)
        self.writeContents(temporaryWriteStream)
        del temporaryWriteStream
        self.writeContents(self.streams['MemoryStream'])

        for (name, stream) in self.streams.items():
            if not stream.canRead():
                continue

            stream.seek(0)
            with self.subTest(name):
                for v in CommonStreamTest.contents:
                    # TODO: should change when the `readValue` binding makes more sense
                    r = None
                    r = v + v
                    stream.readValue(r)
                    self.assertEqual(r, v)

    # TODO: more read / write tests

class DummyStreamTest(unittest.TestCase):
    def setUp(self):
        self.s = DummyStream()

    def test01_basics(self):
        self.assertTrue(self.s.canWrite())
        self.assertFalse(self.s.canRead())

    def test02_str(self):
        self.s.writeValue("hello world")
        # string length as a uint32_t (4) + string (11)
        self.assertEqual(str(self.s),
                        "DummyStream[" +
                        "hostByteOrder=little-endian, byteOrder=little-endian" +
                        ", size=15, pos=15]")


class FileStreamTest(unittest.TestCase):
    roPath = path("./test_file_read")
    woPath = path("./test_file_write")
    newPath = path("./path_that_did_not_exist")

    def setUp(self):
        touch(FileStreamTest.roPath)
        touch(FileStreamTest.woPath)
        if PyPath.exists(str(FileStreamTest.newPath)):
            os.remove(str(FileStreamTest.newPath))

        # Provide read-only and write-only FileStream instances on fresh files
        self.ro = FileStream(FileStreamTest.roPath, False)
        self.wo = FileStream(FileStreamTest.woPath, True)

    def tearDown(self):
        os.remove(str(FileStreamTest.roPath))
        os.remove(str(FileStreamTest.woPath))
        if PyPath.exists(str(FileStreamTest.newPath)):
            os.remove(str(FileStreamTest.newPath))

        # w = FileStream(path('./secret_hello'), True)
        # w.seek(0)
        # w.writeValue("hello world")
        # w.flush()

    def test01_basics(self):
        self.assertTrue(self.ro.canRead())
        self.assertFalse(self.ro.canWrite())
        self.assertFalse(self.wo.canRead())
        self.assertTrue(self.wo.canWrite())

        # Read / write modes should be enforced
        with self.assertRaises(Exception):
            self.ro.writeValue("hello")
        with self.assertRaises(Exception):
            v = 0
            self.wo.readValue(v)

    def test02_create_on_open(self):
        p = FileStreamTest.newPath
        # File should only be created when opening in write mode
        self.assertFalse(PyPath.exists(str(p)))
        _ = FileStream(p, True)
        self.assertTrue(PyPath.exists(str(p)))
        os.remove(str(p))
        # In read-only mode, throws if the file doesn't exist
        self.assertFalse(PyPath.exists(str(p)))
        with self.assertRaises(Exception):
            _ = FileStream(p, False)
        self.assertFalse(PyPath.exists(str(p)))

    def test03_truncate(self):
        # Cannot truncate a read-only stream
        with self.assertRaises(Exception):
            self.ro.truncate(5)

        # Truncating shouldn't change the position if not necessary
        self.assertEqual(self.wo.getPos(), 0)
        self.wo.truncate(5)
        self.assertEqual(self.wo.getPos(), 0)
        self.wo.writeValue("hello")
        self.wo.flush()
        self.assertEqual(self.wo.getPos(), 0 + 9) # TODO: why not 9? (= 4 + 5)
        self.wo.truncate(5)
        self.assertEqual(self.wo.getPos(), 5)

    def test04_seek(self):
        self.wo.truncate(5)
        self.wo.seek(5)
        self.assertEqual(self.wo.getPos(), 5)
        self.assertEqual(self.wo.getSize(), 5)
        self.wo.writeValue("hello world")
        self.wo.flush()
        self.wo.seek(3)
        self.assertEqual(self.wo.getPos(), 3)
        self.assertEqual(self.wo.getSize(), 5+4+11)
        self.wo.writeValue("dlrow olleh")
        self.wo.flush()
        self.assertEqual(self.wo.getPos(), 3+4+11)
        self.assertEqual(self.wo.getSize(), 5+4+11)

        # Seeking further that the limit of the file should be okay too, but
        # not update the size.
        self.ro.seek(10)
        self.assertEqual(self.ro.getPos(), 10)
        self.assertEqual(self.ro.getSize(), 0)

        self.wo.seek(40)
        self.assertEqual(self.wo.getPos(), 40)
        self.assertEqual(self.wo.getSize(), 5+4+11) # Actual size

        # A subsequent write should start at the correct position
        self.wo.writeValue(13.37) # Float (1)
        self.assertEqual(self.wo.getPos(), 40 + 1)
        self.assertEqual(self.wo.getSize(), 40 + 1)

    def test05_str(self):
        self.assertEqual(str(self.ro),
                         "FileStream[" +
                         "hostByteOrder=little-endian, byteOrder=little-endian" +
                         ", path=" + str(FileStreamTest.roPath) +
                         ", writeOnly=false]")

        self.wo.writeValue("hello world")
        self.assertEqual(str(self.wo),
                         "FileStream[" +
                         "hostByteOrder=little-endian, byteOrder=little-endian" +
                         ", path=" + str(FileStreamTest.woPath) +
                         ", writeOnly=true]")


class MemoryStreamTest(unittest.TestCase):
    defaultCapacity = 64

    def setUp(self):
        # Provide a fresh MemoryStream instance
        self.s = MemoryStream(MemoryStreamTest.defaultCapacity)

    def test01_basics(self):
        self.assertTrue(self.s.canWrite())
        self.assertTrue(self.s.canRead())

    def test02_str(self):
        self.s.writeValue("hello world")
        # string length as a uint32_t (4) + string (11)
        self.assertEqual(str(self.s),
                         "MemoryStream[" +
                         "hostByteOrder=little-endian, byteOrder=little-endian" +
                         ", ownsBuffer=1, capacity=" + str(MemoryStreamTest.defaultCapacity) +
                         ", size=15, pos=15]")

if __name__ == '__main__':
    unittest.main()
