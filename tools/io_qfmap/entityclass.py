# vim:ts=4:et
# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

import os
from .script import Script
from .qfplist import pldata

MAX_FLAGS = 8

class EntityClass:
    def __init__(self, name, color, size, flagnames, comment):
        self.name = name
        self.color = color
        self.size = size
        self.flagnames = flagnames
        self.comment = comment
    @classmethod
    def null(cls):
        return cls('', (1, 1, 1), None, (), "")
    @classmethod
    def from_quaked(cls, text, filename, line = 0):
        script = Script(filename, text)
        if line:
            script.line = line
        script.getToken()   # skip over the leading '/*QUAKED'
        name = script.getToken()
        color = cls.parse_vector(script)
        size = cls.parse_size(script)
        flagnames = cls.parse_flags(script)
        comment = cls.extract_comment(script)
        return cls(name, color, size, flagnames, comment)
    @classmethod
    def from_dictionary(cls, name, d):
        if "color" in d:
            color = d["color"]
            color = float(color[0]), float(color[1]), float(color[2])
        else:
            color = (0.0, 0.0, 0.0)
        if "size" in d:
            mins, maxs = d["size"]
            size = ((float(mins[0]), float(mins[1]), float(mins[2])),
                    (float(maxs[0]), float(maxs[1]), float(maxs[2])))
        else:
            size = None
        if "flagnames" in d:
            flagnames = tuple(d["flagnames"])
        else:
            flagnames = ()
        if "comment" in d:
            comment = d["comment"]
        else:
            comment = ""
        return cls(name, color, size, flagnames, comment)
    def to_dictionary(self):
        d = {"color":self.color, "flagnames":self.flagnames,
             "comment":self.comment}
        if self.size:
            d["size"] = self.size
        return d
    @classmethod
    def parse_vector(cls, script):
        if script.getToken() != "(":
            raise SyntaxError
        v = (float(script.getToken()), float(script.getToken()),
             float(script.getToken()))
        if script.getToken() != ")":
            raise SyntaxError
        return v
    @classmethod
    def parse_size(cls, script):
        if script.getToken() == "?":
            return None                 # use brush size
        script.ungetToken()
        return cls.parse_vector(script), cls.parse_vector(script)
    @classmethod
    def parse_flags(cls, script):
        flagnames = []
        while script.tokenAvailable():
            #any remaining words on the line are flag names, but only MAX_FLAGS
            #names are kept.
            script.getToken()
            if len(flagnames) < MAX_FLAGS:
                flagnames.append(script.token)
        return tuple(flagnames)
    @classmethod
    def extract_comment(cls, script):
        if not script.tokenAvailable(True):
            return ""
        start = pos = script.pos
        while pos < len(script.text) and script.text[pos:pos + 2] != "*/":
            if script.text[pos] == "\n":
                script.line += 1
            pos += 1
        comment = script.text[start:pos]
        if pos < len(script.text):
            pos += 2
        script.pos = pos
        return comment

class EntityClassDict:
    def __init__(self):
        self.path = ""
        self.entity_classes = {}
    def scan_source(self, fname):
        text = open(fname, "rt").read()
        line = 1
        pos = 0
        while pos < len(text):
            if text[pos:pos + 8] == "/*QUAKED":
                start = pos
                start_line = line
                while pos < len(text) and text[pos:pos + 2] != "*/":
                    if text[pos] == "\n":
                        line += 1
                    pos += 1
                if pos < len(text):
                    pos += 2
                ec = EntityClass.from_quaked(text[start:pos], fname,
                                             start_line)
                self.entity_classes[ec.name] = ec
            else:
                if text[pos] == "\n":
                    line += 1
                pos += 1
    def scan_directory(self, path):
        files = os.listdir(path)
        files.sort()
        for f in files:
            if f[0] in [".", "_"]:
                continue
            if os.path.isdir(os.path.join(path, f)):
                self.scan_directory(os.path.join(path, f))
            else:
                if f[-3:] == ".qc":
                    self.scan_source(os.path.join(path, f))
    def from_source_tree(self, path):
        self.path = path
        self.entity_classes = {}
        self.scan_directory(self.path)
    def to_plist(self):
        pl = pldata()
        ec = {}
        for k in self.entity_classes.keys():
            ec[k] = self.entity_classes[k].to_dictionary()
        return pl.write(ec)
    def from_plist(self, plist):
        pl = pldata(plist)
        ec = pl.parse()
        self.entity_classes = {}
        for k in ec.keys():
            self.entity_classes[k] = EntityClass.from_dictionary(k, ec[k])