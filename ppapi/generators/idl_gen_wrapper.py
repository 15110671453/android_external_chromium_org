# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base class for generating wrapper functions for PPAPI methods.
"""

from datetime import datetime
import os
import sys

from idl_c_proto import CGen
from idl_generator import Generator
from idl_log import ErrOut, InfoOut, WarnOut
from idl_outfile import IDLOutFile


class PPKind(object):
  @staticmethod
  def ChoosePPFunc(iface, ppb_func, ppp_func):
    name = iface.node.GetName()
    if name.startswith("PPP"):
      return ppp_func
    elif name.startswith("PPB"):
      return ppb_func
    else:
      raise Exception('Unknown PPKind for ' + name)


class Interface(object):
  """Tracks information about a particular interface version.

  - struct_name: the struct type used by the ppapi headers to hold the
  method pointers (the vtable).
  - needs_wrapping: True if a method in the interface needs wrapping.
  - header_file: the name of the header file that defined this interface.
  """
  def __init__(self, interface_node, release, version,
               struct_name, needs_wrapping, header_file):
    self.node = interface_node
    self.release = release
    self.version = version
    self.struct_name = struct_name
    # We may want finer grained filtering (method level), but it is not
    # yet clear how to actually do that.
    self.needs_wrapping = needs_wrapping
    self.header_file = header_file


class WrapperGen(Generator):
  """WrapperGen - An abstract class that generates wrappers for PPAPI methods.

  This generates a wrapper PPB and PPP GetInterface, which directs users
  to wrapper PPAPI methods. Wrapper PPAPI methods may perform arbitrary
  work before invoking the real PPAPI method (supplied by the original
  GetInterface functions).

  Subclasses must implement GenerateWrapperForPPBMethod (and PPP).
  Optionally, subclasses can implement InterfaceNeedsWrapper to
  filter out interfaces that do not actually need wrappers (those
  interfaces can jump directly to the original interface functions).
  """

  def __init__(self, wrapper_prefix, s1, s2, s3):
    Generator.__init__(self, s1, s2, s3)
    self.wrapper_prefix = wrapper_prefix
    self._skip_opt = False
    self.output_file = None
    self.cgen = CGen()

  def SetOutputFile(self, fname):
    self.output_file = fname


  def GenerateRelease(self, ast, release, options):
    return self.GenerateRange(ast, [release], options)


  @staticmethod
  def GetHeaderName(name):
    """Get the corresponding ppapi .h file from each IDL filename.
    """
    name = os.path.splitext(name)[0] + '.h'
    return 'ppapi/c/' + name


  def WriteCopyrightGeneratedTime(self, out):
    now = datetime.now()
    c = """/* Copyright (c) %s The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Last generated from IDL: %s. */
""" % (now.year, datetime.ctime(now))
    out.Write(c)

  def GetWrapperMetadataName(self):
    return '__%sWrapperInfo' % self.wrapper_prefix


  def GenerateHelperFunctions(self, out):
    """Generate helper functions to avoid dependencies on libc.
    """
    out.Write("""/* Use local strcmp to avoid dependency on libc. */
static int mystrcmp(const char* s1, const char *s2) {
  while((*s1 && *s2) && (*s1++ == *s2++));
  return *(--s1) - *(--s2);
}\n
""")


  def GenerateFixedFunctions(self, out):
    """Write out the set of constant functions (those that do not depend on
    the current Pepper IDL).
    """
    out.Write("""

static PPB_GetInterface __real_PPBGetInterface;
static PPP_GetInterface_Type __real_PPPGetInterface;

void __set_real_%(wrapper_prefix)s_PPBGetInterface(PPB_GetInterface real) {
  __real_PPBGetInterface = real;
}

void __set_real_%(wrapper_prefix)s_PPPGetInterface(PPP_GetInterface_Type real) {
  __real_PPPGetInterface = real;
}

/* Map interface string -> wrapper metadata */
static struct %(wrapper_struct)s *%(wrapper_prefix)sPPBShimIface(
    const char *name) {
  struct %(wrapper_struct)s **next = s_ppb_wrappers;
  while (*next != NULL) {
    if (mystrcmp(name, (*next)->iface_macro) == 0) return *next;
    ++next;
  }
  return NULL;
}

/* Map interface string -> wrapper metadata */
static struct %(wrapper_struct)s *%(wrapper_prefix)sPPPShimIface(
    const char *name) {
  struct %(wrapper_struct)s **next = s_ppp_wrappers;
  while (*next != NULL) {
    if (mystrcmp(name, (*next)->iface_macro) == 0) return *next;
    ++next;
  }
  return NULL;
}

const void *__%(wrapper_prefix)s_PPBGetInterface(const char *name) {
  struct %(wrapper_struct)s *wrapper = %(wrapper_prefix)sPPBShimIface(name);
  if (wrapper == NULL) {
    /* We don't have an IDL for this, for some reason. Take our chances. */
    return (*__real_PPBGetInterface)(name);
  }

  /* Initialize the real_iface if it hasn't been. The wrapper depends on it. */
  if (wrapper->real_iface == NULL) {
    const void *iface = (*__real_PPBGetInterface)(name);
    if (NULL == iface) return NULL;
    wrapper->real_iface = iface;
  }

  if (wrapper->wrapped_iface) {
    return wrapper->wrapped_iface;
  } else {
    return wrapper->real_iface;
  }
}

const void *__%(wrapper_prefix)s_PPPGetInterface(const char *name) {
  struct %(wrapper_struct)s *wrapper = %(wrapper_prefix)sPPPShimIface(name);
  if (wrapper == NULL) {
    /* We don't have an IDL for this, for some reason. Take our chances. */
    return (*__real_PPPGetInterface)(name);
  }

  /* Initialize the real_iface if it hasn't been. The wrapper depends on it. */
  if (wrapper->real_iface == NULL) {
    const void *iface = (*__real_PPPGetInterface)(name);
    if (NULL == iface) return NULL;
    wrapper->real_iface = iface;
  }

  if (wrapper->wrapped_iface) {
    return wrapper->wrapped_iface;
  } else {
    return wrapper->real_iface;
  }
}
""" % { 'wrapper_struct' : self.GetWrapperMetadataName(),
        'wrapper_prefix' : self.wrapper_prefix,
        } )


  ############################################################

  def InterfaceNeedsWrapper(self, iface, releases):
    """Return true if the interface has ANY methods that need wrapping.
    """
    return True


  def OwnHeaderFile(self):
    """Return the header file that specifies the API of this wrapper.
    We do not generate the header files.  """
    raise Exception('Child class must implement this')


  ############################################################

  def DetermineInterfaces(self, ast, releases):
    """Get a list of interfaces along with whatever metadata we need.
    """
    iface_releases = []
    for filenode in ast.GetListOf('File'):
      # If this file has errors, skip it
      if filenode in self.skip_list:
        InfoOut.Log('WrapperGen: Skipping %s due to errors\n' %
                    filenode.GetName())
        continue

      file_name = self.GetHeaderName(filenode.GetName())
      ifaces = filenode.GetListOf('Interface')
      for iface in ifaces:
        releases_for_iface = iface.GetUniqueReleases(releases)
        for release in releases_for_iface:
          version = iface.GetVersion(release)
          struct_name = self.cgen.GetStructName(iface, release,
                                                include_version=True)
          needs_wrap = self.InterfaceVersionNeedsWrapping(iface, version)
          if not needs_wrap:
            InfoOut.Log('Interface %s ver %s does not need wrapping' %
                        (struct_name, version))
          iface_releases.append(
              Interface(iface, release, version,
                        struct_name, needs_wrap, file_name))
    return iface_releases


  def GenerateIncludes(self, iface_releases, out):
    """Generate the list of #include that define the original interfaces.
    """
    self.WriteCopyrightGeneratedTime(out)
    # First include own header.
    out.Write('#include "%s"\n\n' % self.OwnHeaderFile())

    # Get typedefs for PPB_GetInterface.
    out.Write('#include "%s"\n' % self.GetHeaderName('ppb.h'))

    # Get a conservative list of all #includes that are needed,
    # whether it requires wrapping or not. We currently depend on the macro
    # string for comparison, even when it is not wrapped, to decide when
    # to use the original/real interface.
    header_files = set()
    for iface in iface_releases:
      header_files.add(iface.header_file)
    for header in sorted(header_files):
      out.Write('#include "%s"\n' % header)
    out.Write('\n')


  def WrapperMethodPrefix(self, iface, release):
    return '%s_%s_%s_' % (self.wrapper_prefix, release, iface.GetName())


  def GetReturnArgs(self, ret_type, args_spec):
    if ret_type != 'void':
      ret = 'return '
    else:
      ret = ''
    if args_spec:
      args = []
      for arg in args_spec:
        args.append(arg[1])
      args = ', '.join(args)
    else:
      args = ''
    return (ret, args)


  def GenerateWrapperForPPBMethod(self, iface, member):
    result = []
    func_prefix = self.WrapperMethodPrefix(iface.node, iface.release)
    sig = self.cgen.GetSignature(member, iface.release, 'store',
                                 func_prefix, False)
    result.append('static %s {\n' % sig)
    result.append(' while(1) { /* Not implemented */ } \n')
    result.append('}\n')
    return result


  def GenerateWrapperForPPPMethod(self, iface, member):
    result = []
    func_prefix = self.WrapperMethodPrefix(iface.node, iface.release)
    sig = self.cgen.GetSignature(member, iface.release, 'store',
                                 func_prefix, False)
    result.append('static %s {\n' % sig)
    result.append(' while(1) { /* Not implemented */ } \n')
    result.append('}\n')
    return result


  def GenerateWrapperForMethods(self, iface_releases, comments=True):
    """Return a string representing the code for each wrapper method
    (using a string rather than writing to the file directly for testing.)
    """
    result = []
    for iface in iface_releases:
      if not iface.needs_wrapping:
        if comments:
          result.append('/* Not generating wrapper methods for %s */\n\n' %
                        iface.struct_name)
        continue
      if comments:
        result.append('/* Begin wrapper methods for %s */\n\n' %
                      iface.struct_name)
      generator =  PPKind.ChoosePPFunc(iface,
                                       self.GenerateWrapperForPPBMethod,
                                       self.GenerateWrapperForPPPMethod)
      for member in iface.node.GetListOf('Member'):
        # Skip the method if it's not actually in the release.
        if not member.InReleases([iface.release]):
          continue
        result.extend(generator(iface, member))
      if comments:
        result.append('/* End wrapper methods for %s */\n\n' %
                      iface.struct_name)
    return ''.join(result)


  def GenerateWrapperInterfaces(self, iface_releases, out):
    for iface in iface_releases:
      if not iface.needs_wrapping:
        out.Write('/* Not generating wrapper interface for %s */\n\n' %
                  iface.struct_name)
        continue

      out.Write('struct %s %s_Wrappers_%s = {\n' % (iface.struct_name,
                                                    self.wrapper_prefix,
                                                    iface.struct_name))
      methods = []
      for member in iface.node.GetListOf('Member'):
        # Skip the method if it's not actually in the release.
        if not member.InReleases([iface.release]):
          continue
        prefix = self.WrapperMethodPrefix(iface.node, iface.release)
        cast = self.cgen.GetSignature(member, iface.release, 'return',
                                      prefix='',
                                      func_as_ptr=True,
                                      ptr_prefix='',
                                      include_name=False)
        methods.append('  .%s = (%s)&%s%s' % (member.GetName(),
                                              cast,
                                              prefix,
                                              member.GetName()))
      out.Write('  ' + ',\n  '.join(methods) + '\n')
      out.Write('};\n\n')


  def GetWrapperInfoName(self, iface):
    return '%s_WrapperInfo_%s' % (self.wrapper_prefix, iface.struct_name)


  def GenerateWrapperInfoAndCollection(self, iface_releases, out):
    for iface in iface_releases:
      iface_macro = self.cgen.GetInterfaceMacro(iface.node, iface.version)
      if iface.needs_wrapping:
        wrap_iface = '(void *) &%s_Wrappers_%s' % (self.wrapper_prefix,
                                                   iface.struct_name)
      else:
        wrap_iface = 'NULL /* Still need slot for real_iface */'
      out.Write("""static struct %s %s = {
  .iface_macro = %s,
  .wrapped_iface = %s,
  .real_iface = NULL
};\n\n""" % (self.GetWrapperMetadataName(),
             self.GetWrapperInfoName(iface),
             iface_macro,
             wrap_iface))

    # Now generate NULL terminated arrays of the above wrapper infos.
    ppb_wrapper_infos = []
    ppp_wrapper_infos = []
    for iface in iface_releases:
      appender = PPKind.ChoosePPFunc(iface,
                                     ppb_wrapper_infos.append,
                                     ppp_wrapper_infos.append)
      appender('  &%s' % self.GetWrapperInfoName(iface))
    ppb_wrapper_infos.append('  NULL')
    ppp_wrapper_infos.append('  NULL')
    out.Write(
        'static struct %s *s_ppb_wrappers[] = {\n%s\n};\n\n' %
        (self.GetWrapperMetadataName(), ',\n'.join(ppb_wrapper_infos)))
    out.Write(
        'static struct %s *s_ppp_wrappers[] = {\n%s\n};\n\n' %
        (self.GetWrapperMetadataName(), ',\n'.join(ppp_wrapper_infos)))


  def DeclareWrapperInfos(self, iface_releases, out):
    """The wrapper methods usually need access to the real_iface, so we must
    declare these wrapper infos ahead of time (there is a circular dependency).
    """
    out.Write('/* BEGIN Declarations for all Wrapper Infos */\n\n')
    for iface in iface_releases:
      out.Write('static struct %s %s;\n' %
                (self.GetWrapperMetadataName(), self.GetWrapperInfoName(iface)))
    out.Write('/* END Declarations for all Wrapper Infos. */\n\n')


  def GenerateRange(self, ast, releases, options):
    """Generate shim code for a range of releases.
    """

    # Remember to set the output filename before running this.
    out_filename = self.output_file
    if out_filename is None:
      ErrOut.Log('Did not set filename for writing out wrapper\n')
      return 1

    InfoOut.Log("Generating %s for %s" % (out_filename, self.wrapper_prefix))

    out = IDLOutFile(out_filename)

    # Get a list of all the interfaces along with metadata.
    iface_releases = self.DetermineInterfaces(ast, releases)

    # Generate the includes.
    self.GenerateIncludes(iface_releases, out)

    out.Write(self.GetGuardStart())

    # Write out static helper functions (mystrcmp).
    self.GenerateHelperFunctions(out)

    # Declare list of WrapperInfo before actual wrapper methods, since
    # they reference each other.
    self.DeclareWrapperInfos(iface_releases, out)

    # Generate wrapper functions for each wrapped method in the interfaces.
    result = self.GenerateWrapperForMethods(iface_releases)
    out.Write(result)

    # Collect all the wrapper functions into interface structs.
    self.GenerateWrapperInterfaces(iface_releases, out)

    # Generate a table of the wrapped interface structs that can be looked up.
    self.GenerateWrapperInfoAndCollection(iface_releases, out)

    # Write out the IDL-invariant functions.
    self.GenerateFixedFunctions(out)

    out.Write(self.GetGuardEnd())
    out.Close()
    return 0
